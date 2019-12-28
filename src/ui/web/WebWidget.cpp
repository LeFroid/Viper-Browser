#include "AdBlockManager.h"
#include "BrowserTabWidget.h"
#include "FaviconManager.h"
#include "HistoryManager.h"
#include "HttpRequest.h"
#include "MainWindow.h"
#include "Settings.h"
#include "WebWidget.h"
#include "WebHistory.h"
#include "WebInspector.h"
#include "WebLoadObserver.h"
#include "WebPage.h"
#include "WebPageThumbnailStore.h"
#include "WebView.h"

#include <chrono>
#include <QAction>
#include <QHideEvent>
#include <QMouseEvent>
#include <QQuickWidget>
#include <QShowEvent>
#include <QTimer>
#include <QVBoxLayout>

#include <QDebug>

WebWidget::WebWidget(const ViperServiceLocator &serviceLocator, bool privateMode, QWidget *parent) :
    QWidget(parent),
    m_serviceLocator(serviceLocator),
    m_adBlockManager(serviceLocator.getServiceAs<adblock::AdBlockManager>("AdBlockManager")),
#if (QTWEBENGINECORE_VERSION >= QT_VERSION_CHECK(5, 14, 0))
    m_lifecycleFreezeTimerId(-1),
    m_lifecycleDiscardTimerId(-1),
#endif
    m_page(nullptr),
    m_view(nullptr),
    m_inspector(nullptr),
    m_pageLoadObserver(nullptr),
    m_mainWindow(nullptr),
    m_faviconManager(serviceLocator.getServiceAs<FaviconManager>("FaviconManager")),
    m_privateMode(privateMode),
    m_contextMenuPosGlobal(),
    m_contextMenuPosRelative(),
    m_viewFocusProxy(nullptr),
    m_hibernating(false),
    m_savedState(),
    m_lastTypedUrl()
{
    setObjectName(QLatin1String("webWidget"));

    m_mainWindow = qobject_cast<MainWindow*>(window());

    setupWebView();

    QVBoxLayout *vLayout = new QVBoxLayout(this);
    vLayout->setContentsMargins(0, 0, 0, 0);
    vLayout->setSpacing(0);
    vLayout->addWidget(m_view);
    setLayout(vLayout);

    setFocusProxy(m_view);

    if (BrowserTabWidget *tabWidget = qobject_cast<BrowserTabWidget*>(parentWidget()))
    {
        connect(tabWidget, &BrowserTabWidget::tabPinned, this, &WebWidget::onTabPinned);
    }

    if (!m_privateMode)
    {
        if (HistoryManager *historyMgr = serviceLocator.getServiceAs<HistoryManager>("HistoryManager"))
            m_pageLoadObserver = new WebLoadObserver(historyMgr, this);

        if (WebPageThumbnailStore *thumbnailStore = serviceLocator.getServiceAs<WebPageThumbnailStore>("WebPageThumbnailStore"))
            connect(this, &WebWidget::loadFinished, thumbnailStore, &WebPageThumbnailStore::onPageLoaded);
    }
}

WebWidget::~WebWidget()
{
    if (m_inspector)
    {
        delete m_inspector;
        m_inspector = nullptr;
    }
}

int WebWidget::getProgress() const
{
    if (m_hibernating)
        return 100;

    return m_view->getProgress();
}

void WebWidget::loadBlankPage()
{
    if (!m_hibernating)
        m_view->loadBlankPage();
}

bool WebWidget::isHibernating() const
{
    return m_hibernating;
}

bool WebWidget::isOnBlankPage() const
{
    if (m_hibernating)
        return false;

    return m_view->isOnBlankPage();
}

QIcon WebWidget::getIcon() const
{
    if (m_hibernating)
        return m_savedState.icon;

    QIcon icon { m_page->icon() };

    if (icon.isNull() && m_faviconManager != nullptr)
        return m_faviconManager->getFavicon(m_page->url());

    return icon;
}

QUrl WebWidget::getIconUrl() const
{
    if (m_hibernating)
        return m_savedState.iconUrl;

    return m_page->iconUrl();
}

QString WebWidget::getTitle() const
{
    if (m_hibernating)
        return m_savedState.title;

    return m_view->getTitle();
}

const WebState &WebWidget::getState()
{
    if (!m_hibernating)
        saveState();

    return m_savedState;
}

void WebWidget::load(const QUrl &url, bool wasEnteredByUser)
{
    if (m_hibernating)
        setHibernation(false);

    if (wasEnteredByUser)
        m_lastTypedUrl = url;

    m_view->load(url);
}

void WebWidget::load(const HttpRequest &request)
{
    if (m_hibernating)
        setHibernation(false);

    m_view->load(request);
}

void WebWidget::reload()
{
    if (m_hibernating)
    {
        setHibernation(false);
        return;
    }

    m_page->triggerAction(WebPage::Reload);
}

void WebWidget::stop()
{
    if (!m_hibernating)
        m_view->stop();
}

void WebWidget::setHibernation(bool on)
{
    if (m_hibernating == on)
        return;

    if (on)
    {
        emit aboutToHibernate();
        saveState();

        layout()->removeWidget(m_view);
        setFocusProxy(nullptr);

        if (m_inspector)
        {
            delete m_inspector;
            m_inspector = nullptr;
        }

        delete m_view;
        m_view = nullptr;
        m_page = nullptr;

        setCursor(Qt::PointingHandCursor);
        setAutoFillBackground(true);
    }
    else
    {
        m_hibernating = false;
        setAutoFillBackground(false);

        setupWebView();
        layout()->addWidget(m_view);
        setFocusProxy(m_view);

        emit aboutToWake();
        m_view->load(m_savedState.url);
        getHistory()->load(m_savedState.pageHistory);

        unsetCursor();

        QTimer::singleShot(500, this, [this](){
            if (m_viewFocusProxy.isNull())
                m_viewFocusProxy = m_view->getViewFocusProxy();
        });
    }

    m_hibernating = on;
}

void WebWidget::setWebState(WebState &&state)
{
    m_savedState = state;

    if (!m_hibernating)
    {
        load(state.url);
        getHistory()->load(state.pageHistory);
    }
    else
        emit loadFinished(false);
}

const QUrl &WebWidget::getLastTypedUrl() const
{
    return m_lastTypedUrl;
}

void WebWidget::clearLastTypedUrl()
{
    m_lastTypedUrl = QUrl();
}

WebHistory *WebWidget::getHistory() const
{
    if (m_hibernating)
        return nullptr;

    return m_page->getHistory();
}

QByteArray WebWidget::getEncodedHistory() const
{
    if (m_hibernating)
        return m_savedState.pageHistory;

    return m_page->getHistory()->save();
}

bool WebWidget::isInspectorActive() const
{
    if (m_hibernating)
        return false;

    return m_inspector ? m_inspector->isActive() : false;
}

WebInspector *WebWidget::getInspector()
{
    if (!m_inspector && !m_hibernating)
    {
        m_inspector = new WebInspector(m_privateMode);
        m_inspector->setupPage(m_serviceLocator);
#if (QTWEBENGINECORE_VERSION >= QT_VERSION_CHECK(5, 11, 0))
        m_inspector->page()->setInspectedPage(m_page);
#else
    if (Settings *settings = m_serviceLocator.getServiceAs<Settings>("Settings"))
    {
        QString inspectorUrl = QString("http://127.0.0.1:%1").arg(settings->getValue(BrowserSetting::InspectorPort).toString());
        m_inspector->load(QUrl(inspectorUrl));
    }
#endif
    }
    return m_inspector;
}

QUrl WebWidget::url() const
{
    if (m_hibernating)
        return m_savedState.url;

    return m_page->url();
}

QUrl WebWidget::getOriginalUrl() const
{
    if (m_hibernating)
        return m_savedState.url;

    return m_page->getOriginalUrl();
}

WebPage *WebWidget::page() const
{
    if (m_hibernating)
        return nullptr;

    return m_page;
}

WebView *WebWidget::view() const
{
    if (m_hibernating)
        return nullptr;

    return m_view;
}

void WebWidget::mousePressEvent(QMouseEvent *event)
{
    if (m_hibernating)
    {
        event->accept();
        setHibernation(false);
        return;
    }

    QWidget::mousePressEvent(event);
}

#if (QTWEBENGINECORE_VERSION >= QT_VERSION_CHECK(5, 14, 0))

void WebWidget::hideEvent(QHideEvent *event)
{
    using namespace std::chrono_literals;
    QWidget::hideEvent(event);

    if (!m_hibernating && m_view)
        m_view->hideEvent(event);

    if (m_lifecycleFreezeTimerId == -1 && m_lifecycleDiscardTimerId == -1)
        m_lifecycleFreezeTimerId = startTimer(60s);
}

void WebWidget::showEvent(QShowEvent *event)
{
    const bool updateWebContents = !m_hibernating && m_view && m_page;
    if (updateWebContents)
    {
        if (m_page->lifecycleState() != WebPage::LifecycleState::Active)
        {
            if (m_page->lifecycleState() == WebPage::LifecycleState::Discarded)
            {
                QTimer::singleShot(50, this, [this](){
                    hide();
                    show();
                });
            }
            m_page->setLifecycleState(WebPage::LifecycleState::Active);
        }

        if (m_lifecycleFreezeTimerId != -1)
        {
            killTimer(m_lifecycleFreezeTimerId);
            m_lifecycleFreezeTimerId = -1;
        }

        if (m_lifecycleDiscardTimerId != -1)
        {
            killTimer(m_lifecycleDiscardTimerId);
            m_lifecycleDiscardTimerId = -1;
        }
    }

    QWidget::showEvent(event);
}

void WebWidget::timerEvent(QTimerEvent *event)
{
    const int timerId = event->timerId();
    if (timerId == m_lifecycleFreezeTimerId || timerId == m_lifecycleDiscardTimerId)
    {
        killTimer(timerId);

        WebPage::LifecycleState targetState;
        if (timerId == m_lifecycleFreezeTimerId)
        {
            targetState = WebPage::LifecycleState::Frozen;
            m_lifecycleFreezeTimerId = -1;
        }
        else
        {
            targetState = WebPage::LifecycleState::Discarded;
            m_lifecycleDiscardTimerId = -1;
        }

        if (!isHidden()
                || m_hibernating
                || m_view == nullptr
                || m_page == nullptr)
        {
            return;
        }

        if (m_inspector != nullptr
                && m_inspector->page()->inspectedPage() == static_cast<QWebEnginePage*>(m_page))
            return;

        m_page->setLifecycleState(targetState);

        if (targetState == WebPage::LifecycleState::Frozen)
        {
            using namespace std::chrono_literals;
            m_lifecycleDiscardTimerId = startTimer(45min);
        }
    }
    else
        QWidget::timerEvent(event);
}

#endif

void WebWidget::showContextMenuForView()
{
    if (!m_hibernating)
        m_view->showContextMenu(m_contextMenuPosGlobal, m_contextMenuPosRelative);
}

void WebWidget::onTabPinned(int index, bool value)
{
    if (BrowserTabWidget *tabWidget = qobject_cast<BrowserTabWidget*>(parentWidget()))
    {
        if (tabWidget->getWebWidget(index) == this)
            m_savedState.isPinned = value;
    }
}

void WebWidget::saveState()
{
    BrowserTabWidget *tabWidget = qobject_cast<BrowserTabWidget*>(parentWidget());
    m_savedState = WebState(this, tabWidget);
}

void WebWidget::setupWebView()
{
#if (QTWEBENGINECORE_VERSION >= QT_VERSION_CHECK(5, 14, 0))
    if (m_lifecycleFreezeTimerId != -1)
    {
        killTimer(m_lifecycleFreezeTimerId);
        m_lifecycleFreezeTimerId = -1;
    }

    if (m_lifecycleDiscardTimerId != -1)
    {
        killTimer(m_lifecycleDiscardTimerId);
        m_lifecycleDiscardTimerId = -1;
    }
#endif

    m_viewFocusProxy = nullptr;
    m_view = new WebView(m_privateMode, this);
    m_view->setupPage(m_serviceLocator);
    m_view->installEventFilter(this);

    m_page = m_view->getPage();

    connect(m_page, &WebPage::iconChanged,          this, &WebWidget::iconChanged);
    connect(m_page, &WebPage::iconUrlChanged,       this, &WebWidget::iconUrlChanged);
    connect(m_page, &WebPage::linkHovered,          this, &WebWidget::linkHovered);
    connect(m_page, &WebPage::titleChanged,         this, &WebWidget::titleChanged);
    connect(m_page, &WebPage::windowCloseRequested, this, &WebWidget::closeRequest);
    connect(m_page, &WebPage::urlChanged,           this, &WebWidget::urlChanged);

    connect(m_page, &WebPage::loadStarted, this, [this](){
        m_adBlockManager->loadStarted(m_page->url().adjusted(QUrl::RemoveFragment));
    });

    connect(m_view, &WebView::loadFinished, this, &WebWidget::loadFinished);
    connect(m_view, &WebView::loadProgress, this, &WebWidget::loadProgress);
    connect(m_view, &WebView::openRequest, this, &WebWidget::openRequest);
    connect(m_view, &WebView::openInNewTab, this, &WebWidget::openInNewTab);
    connect(m_view, &WebView::openInNewBackgroundTab, this, &WebWidget::openInNewBackgroundTab);
    connect(m_view, &WebView::openInNewWindowRequest, this, &WebWidget::openInNewWindowRequest);
    connect(m_view, &WebView::inspectElement, this, [this](){
        m_inspector = getInspector();
        emit inspectElement();
    });

    connect(m_view, &WebView::fullScreenRequested, m_mainWindow, &MainWindow::onToggleFullScreen);

    connect(m_view, &WebView::openHttpRequestInBackgroundTab, this, &WebWidget::openHttpRequestInBackgroundTab);
}

bool WebWidget::eventFilter(QObject *watched, QEvent *event)
{
    if (m_hibernating)
        return QWidget::eventFilter(watched, event);

    switch (event->type())
    {
        case QEvent::ChildAdded:
        case QEvent::ChildRemoved:
        {
            if (watched == m_view)
            {
                QTimer::singleShot(0, this, [this](){
                    if (m_hibernating)
                        return;
                    QWidget *proxy = m_view->focusProxy();
                    if (proxy && proxy != m_viewFocusProxy && (qobject_cast<QQuickWidget*>(proxy) != 0))
                    {
                        m_viewFocusProxy = proxy;
                        m_viewFocusProxy->installEventFilter(this);

                        m_view->setViewFocusProxy(m_viewFocusProxy);
                    }
                    else
                    {
                        QWidget *w = m_view->getViewFocusProxy();
                        if (w && w != m_viewFocusProxy)
                        {
                            m_viewFocusProxy = w;
                            m_viewFocusProxy->installEventFilter(this);
                            m_view->setViewFocusProxy(m_viewFocusProxy);
                        }
                    }
                });
            }
            break;
        }
        case QEvent::ContextMenu:
        {
            if (watched == m_viewFocusProxy)
            {
                QContextMenuEvent *contextMenuEvent = static_cast<QContextMenuEvent*>(event);
                m_contextMenuPosGlobal = contextMenuEvent->globalPos();
                m_contextMenuPosRelative = contextMenuEvent->pos();
                QTimer::singleShot(10, this, &WebWidget::showContextMenuForView);
                return true;
            }
            break;
        }
        case QEvent::Wheel:
        {
            if (watched == m_viewFocusProxy || watched == m_view->getViewFocusProxy())
            {
                QWheelEvent *wheelEvent = static_cast<QWheelEvent*>(event);

                const bool wasAccepted =  event->isAccepted();
                event->setAccepted(false);

                m_view->_wheelEvent(wheelEvent);

                const bool isAccepted = wheelEvent->isAccepted();
                event->setAccepted(wasAccepted);
                return isAccepted;
            }
            else if (watched == this || watched == m_view)
            {
                return true;
            }
            break;
        }
        case QEvent::MouseButtonRelease:
        {
            if (watched == m_viewFocusProxy || watched == m_view->getViewFocusProxy())
            {
                QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);

                const bool wasAccepted =  event->isAccepted();
                event->setAccepted(false);

                m_view->_mouseReleaseEvent(mouseEvent);

                const bool isAccepted = mouseEvent->isAccepted();
                event->setAccepted(wasAccepted);
                return isAccepted;
            }
            else if (watched == this && m_hibernating)
                return false;
            else if (watched == this || watched == m_view)
                return true;
            break;
        }
        default:
            break;
    }
    return QWidget::eventFilter(watched, event);
}
