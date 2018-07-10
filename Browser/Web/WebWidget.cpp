#include "AdBlockManager.h"
#include "MainWindow.h"
#include "WebWidget.h"
#include "WebPage.h"
#include "WebView.h"

#include <QFocusEvent>
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
    m_contextMenuPosRelative()
{
    setFocusPolicy(Qt::StrongFocus);
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
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
}

void WebWidget::focusInEvent(QFocusEvent *event)
{
    QWidget::focusInEvent(event);

    if (m_view)
        m_view->setFocus();
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
            if (WebView *view = qobject_cast<WebView*>(watched))
            {
                if (qobject_cast<MainWindow*>(view->window()) != m_mainWindow)
                    return false;

                QChildEvent *childAddedEvent = static_cast<QChildEvent*>(event);
                if (QObject *child = childAddedEvent->child())
                    child->installEventFilter(this);
            }
            break;
        }
        case QEvent::ContextMenu:
        {
            if (WebView *view = qobject_cast<WebView*>(watched))
            {
                if (qobject_cast<MainWindow*>(view->window()) != m_mainWindow)
                    return false;

                QContextMenuEvent *contextMenuEvent = static_cast<QContextMenuEvent*>(event);
                m_contextMenuPosGlobal = contextMenuEvent->globalPos();
                m_contextMenuPosRelative = contextMenuEvent->pos();
                QTimer::singleShot(10, this, &WebWidget::showContextMenuForView);
                return true;
            }
            break;
        }
        default:
            break;
    }
    return QWidget::eventFilter(watched, event);
}
