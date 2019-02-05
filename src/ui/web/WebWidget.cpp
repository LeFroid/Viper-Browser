#include "AdBlockManager.h"
#include "BrowserApplication.h"
#include "BrowserTabWidget.h"
#include "HistoryManager.h"
#include "HttpRequest.h"
#include "MainWindow.h"
#include "WebWidget.h"
#include "WebHistory.h"
#include "WebPage.h"
#include "WebPageThumbnailStore.h"
#include "WebView.h"

#include <QAction>
#include <QMouseEvent>
#include <QTimer>
#include <QVBoxLayout>

WebState::WebState(WebWidget *webWidget, BrowserTabWidget *tabWidget) :
    index(0),
    isPinned(false),
    icon(webWidget->getIcon()),
    iconUrl(webWidget->getIconUrl()),
    title(webWidget->getTitle()),
    url(webWidget->url()),
    pageHistory()
{
    if (tabWidget != nullptr)
    {
        index = tabWidget->indexOf(webWidget);
        isPinned = tabWidget->isTabPinned(index);

        if (icon.isNull())
            icon = tabWidget->tabIcon(index);
    }

    if (WebHistory *history = webWidget->getHistory())
        pageHistory = history->save();
}

QByteArray WebState::serialize() const
{
    QByteArray result;
    QDataStream stream(&result, QIODevice::WriteOnly);
    stream << index
           << isPinned
           << icon
           << iconUrl
           << title
           << url
           << pageHistory;
    return result;
}

void WebState::deserialize(QByteArray &data)
{
    QDataStream stream(&data, QIODevice::ReadOnly);
    stream  >> index
            >> isPinned
            >> icon
            >> iconUrl
            >> title
            >> url
            >> pageHistory;
}

WebWidget::WebWidget(ViperServiceLocator &serviceLocator, bool privateMode, QWidget *parent) :
    QWidget(parent),
    m_serviceLocator(serviceLocator),
    m_adBlockManager(serviceLocator.getServiceAs<AdBlockManager>("AdBlockManager")),
    m_page(nullptr),
    m_view(nullptr),
    m_mainWindow(nullptr),
    m_privateMode(privateMode),
    m_contextMenuPosGlobal(),
    m_contextMenuPosRelative(),
    m_viewFocusProxy(nullptr),
    m_hibernating(false),
    m_savedState()
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

    if (!m_privateMode)
    {
        connect(this, &WebWidget::loadFinished, sBrowserApplication->getHistoryManager(), &HistoryManager::onPageLoaded);
        connect(this, &WebWidget::loadFinished, sBrowserApplication->getWebPageThumbnailStore(), &WebPageThumbnailStore::onPageLoaded);
    }
}

WebWidget::~WebWidget()
{
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

    return m_page->icon();
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
    if (m_hibernating)
        return m_savedState;

    saveState();
    return m_savedState;
}

void WebWidget::load(const QUrl &url)
{
    if (m_hibernating)
        setHibernation(false);

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

    m_page->action(WebPage::Reload)->trigger();
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
        delete m_view;
        m_view = nullptr;
        m_page = nullptr;

        setCursor(Qt::PointingHandCursor);
    }
    else
    {
        m_hibernating = false;

        setupWebView();
        layout()->addWidget(m_view);
        setFocusProxy(m_view);

        emit aboutToWake();
        m_view->load(m_savedState.url);
        getHistory()->load(m_savedState.pageHistory);

        unsetCursor();

        QTimer::singleShot(500, this, [=](){
            m_viewFocusProxy = m_view->getViewFocusProxy();
        });
    }

    m_hibernating = on;
}

void WebWidget::setWebState(WebState &state)
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

void WebWidget::showContextMenuForView()
{
    if (!m_hibernating)
        m_view->showContextMenu(m_contextMenuPosGlobal, m_contextMenuPosRelative);
}

void WebWidget::saveState()
{
    BrowserTabWidget *tabWidget = qobject_cast<BrowserTabWidget*>(parentWidget());
    m_savedState = WebState(this, tabWidget);
}

void WebWidget::setupWebView()
{
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

    connect(m_page, &WebPage::loadStarted, [=](){
        m_adBlockManager->loadStarted(m_page->url().adjusted(QUrl::RemoveFragment));
    });

    connect(m_view, &WebView::loadFinished, this, &WebWidget::loadFinished);
    connect(m_view, &WebView::loadProgress, this, &WebWidget::loadProgress);
    connect(m_view, &WebView::openRequest, this, &WebWidget::openRequest);
    connect(m_view, &WebView::openInNewTab, this, &WebWidget::openInNewTab);
    connect(m_view, &WebView::openInNewBackgroundTab, this, &WebWidget::openInNewBackgroundTab);
    connect(m_view, &WebView::openInNewWindowRequest, this, &WebWidget::openInNewWindowRequest);
    connect(m_view, &WebView::inspectElement, this, &WebWidget::inspectElement);

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
                    if (m_view->focusProxy() && m_view->focusProxy() != m_viewFocusProxy)
                    {
                        m_viewFocusProxy = m_view->focusProxy();
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
