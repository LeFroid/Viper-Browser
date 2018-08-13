#include "AdBlockManager.h"
#include "BrowserTabWidget.h"
#include "HttpRequest.h"
#include "MainWindow.h"
#include "WebWidget.h"
#include "WebPage.h"
#include "WebView.h"

#include <QMouseEvent>
#include <QTimer>
#include <QVBoxLayout>
#include <QWebEngineHistory>

WebWidget::WebWidget(bool privateMode, QWidget *parent) :
    QWidget(parent),
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

    return m_view->icon();
}

QUrl WebWidget::getIconUrl() const
{
    if (m_hibernating)
        return m_savedState.iconUrl;

    return m_view->iconUrl();
}

QString WebWidget::getTitle() const
{
    if (m_hibernating)
        return m_savedState.title;

    return m_view->getTitle();
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

    m_view->reload();
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

    m_hibernating = on;

    if (on)
    {
        emit aboutToHibernate();
        saveState();
        layout()->removeWidget(m_view);
        delete m_view;
        m_view = nullptr;

        setCursor(Qt::PointingHandCursor);
    }
    else
    {
        setupWebView();
        layout()->addWidget(m_view);
        setFocusProxy(m_view);

        emit aboutToWake();
        m_view->load(m_savedState.url);
        QDataStream historyStream(&m_savedState.pageHistory, QIODevice::ReadWrite);
        historyStream >> *(m_view->history());

        unsetCursor();
    }
}

QWebEngineHistory *WebWidget::history() const
{
    if (m_hibernating)
        return nullptr;

    return m_view->history();
}

QUrl WebWidget::url() const
{
    if (m_hibernating)
        return m_savedState.url;

    return m_view->url();
}

WebPage *WebWidget::page() const
{
    if (m_hibernating)
        return nullptr;

    return m_view->getPage();
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
    m_savedState.icon = m_view->icon();
    if (m_savedState.icon.isNull())
    {
        if (BrowserTabWidget *tabWidget = qobject_cast<BrowserTabWidget*>(parentWidget()))
            m_savedState.icon = tabWidget->tabIcon(tabWidget->indexOf(this));
    }

    m_savedState.iconUrl = m_view->iconUrl();

    m_savedState.title = m_view->title();

    m_savedState.url = m_view->url();

    QDataStream stream(&m_savedState.pageHistory, QIODevice::ReadWrite);
    stream << *(m_view->history());
    stream.device()->seek(0);
}

void WebWidget::setupWebView()
{
    m_view = new WebView(m_privateMode, this);
    m_view->setupPage();
    m_view->installEventFilter(this);

    WebPage *page = m_view->getPage();

    connect(page, &WebPage::iconChanged,          this, &WebWidget::iconChanged);
    connect(page, &WebPage::iconUrlChanged,       this, &WebWidget::iconUrlChanged);
    connect(page, &WebPage::linkHovered,          this, &WebWidget::linkHovered);
    connect(page, &WebPage::titleChanged,         this, &WebWidget::titleChanged);
    connect(page, &WebPage::windowCloseRequested, this, &WebWidget::closeRequest);

    connect(page, &WebPage::loadStarted, [=](){
        AdBlockManager::instance().loadStarted(m_view->url());
    });

    connect(m_view, &WebView::loadFinished, this, &WebWidget::loadFinished);
    connect(m_view, &WebView::loadProgress, this, &WebWidget::loadProgress);
    connect(m_view, &WebView::openRequest, this, &WebWidget::openRequest);
    connect(m_view, &WebView::openInNewTab, this, &WebWidget::openInNewTab);
    connect(m_view, &WebView::openInNewBackgroundTab, this, &WebWidget::openInNewBackgroundTab);
    connect(m_view, &WebView::openInNewWindowRequest, this, &WebWidget::openInNewWindowRequest);
    connect(m_view, &WebView::inspectElement, this, &WebWidget::inspectElement);

    connect(m_view, &WebView::fullScreenRequested, m_mainWindow, &MainWindow::onToggleFullScreen);
}

bool WebWidget::eventFilter(QObject *watched, QEvent *event)
{
    if (m_hibernating)
        return QWidget::eventFilter(watched, event);

    switch (event->type())
    {
        case QEvent::ChildAdded:
        {
            if (watched == m_view)
            {
                QTimer::singleShot(0, this, [this](){
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
            if (watched == m_viewFocusProxy)
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
        default:
            break;
    }
    return QWidget::eventFilter(watched, event);
}
