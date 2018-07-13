#include "AdBlockManager.h"
#include "MainWindow.h"
#include "WebWidget.h"
#include "WebPage.h"
#include "WebView.h"

#include <QIcon>
#include <QTimer>
#include <QUrl>
#include <QVBoxLayout>
#include <QWebEngineHistory>

WebWidget::WebWidget(bool privateMode, QWidget *parent) :
    QWidget(parent),
    m_view(nullptr),
    m_mainWindow(nullptr),
    m_privateMode(privateMode),
    m_contextMenuPosGlobal(),
    m_contextMenuPosRelative(),
    m_viewFocusProxy(nullptr)
{
    setObjectName(QLatin1String("webWidget"));
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
}

void WebWidget::setupView()
{
    if (m_view != nullptr)
        return;

    m_mainWindow = qobject_cast<MainWindow*>(window());

    m_view = new WebView(m_privateMode, this);
    m_view->installEventFilter(this);

    connect(m_view, &WebView::iconChanged, this, &WebWidget::iconChanged);
    connect(m_view, &WebView::iconUrlChanged, this, &WebWidget::iconUrlChanged);
    connect(m_view, &WebView::linkHovered, this, &WebWidget::linkHovered);
    connect(m_view, &WebView::loadFinished, this, &WebWidget::loadFinished);
    connect(m_view, &WebView::loadProgress, this, &WebWidget::loadProgress);
    connect(m_view, &WebView::openRequest, this, &WebWidget::openRequest);
    connect(m_view, &WebView::openInNewTabRequest, this, &WebWidget::openInNewTabRequest);
    connect(m_view, &WebView::openInNewWindowRequest, this, &WebWidget::openInNewWindowRequest);
    connect(m_view, &WebView::titleChanged, this, &WebWidget::titleChanged);
    connect(m_view, &WebView::viewCloseRequested, this, &WebWidget::closeRequest);
    connect(m_view, &WebView::inspectElement, this, &WebWidget::inspectElement);

    connect(m_view, &WebView::loadStarted, [=](){
        AdBlockManager::instance().loadStarted(m_view->url());
    });

    connect(m_view, &WebView::fullScreenRequested, m_mainWindow, &MainWindow::onToggleFullScreen);

    QVBoxLayout *vLayout = new QVBoxLayout(this);
    vLayout->setContentsMargins(0, 0, 0, 0);
    vLayout->addWidget(m_view);
    setLayout(vLayout);

    setFocusProxy(m_view);
}

int WebWidget::getProgress() const
{
    return m_view->getProgress();
}

void WebWidget::loadBlankPage()
{
    m_view->loadBlankPage();
}

bool WebWidget::isOnBlankPage() const
{
    return m_view->isOnBlankPage();
}

QIcon WebWidget::getIcon() const
{
    return m_view->icon();
}

QUrl WebWidget::getIconUrl() const
{
    return m_view->iconUrl();
}

QString WebWidget::getTitle() const
{
    return m_view->getTitle();
}

void WebWidget::load(const QUrl &url)
{
    m_view->load(url);
}

void WebWidget::reload()
{
    m_view->reload();
}

void WebWidget::stop()
{
    m_view->stop();
}

QWebEngineHistory *WebWidget::history() const
{
    return m_view->history();
}

QUrl WebWidget::url() const
{
    return m_view->url();
}

WebPage *WebWidget::page() const
{
    return qobject_cast<WebPage*>(m_view->page());
}

WebView *WebWidget::view() const
{
    return m_view;
}

void WebWidget::showContextMenuForView()
{
    m_view->showContextMenu(m_contextMenuPosGlobal, m_contextMenuPosRelative);
}

bool WebWidget::eventFilter(QObject *watched, QEvent *event)
{
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
