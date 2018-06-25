#include "BrowserApplication.h"
#include "BrowserTabWidget.h"
#include "BrowserTabBar.h"
#include "FaviconStorage.h"
#include "MainWindow.h"
#include "WebPage.h"
#include "WebView.h"

#include <algorithm>
#include <QMenu>
#include <QTimer>
#include <QWebEngineHistory>
#include <QWebEngineHistoryItem>

ClosedTabInfo::ClosedTabInfo(int tabIndex, WebView *view) :
    index(tabIndex),
    url(),
    pageHistory()
{
    if (view)
    {
        url = view->url();

        QDataStream stream(&pageHistory, QIODevice::ReadWrite);
        stream << *(view->history());
        stream.device()->seek(0);
    }
}

BrowserTabWidget::BrowserTabWidget(std::shared_ptr<Settings> settings, bool privateMode, QWidget *parent) :
    QTabWidget(parent),
    m_settings(settings),
    m_privateBrowsing(privateMode),
    m_newTabPage(settings->getValue("NewTabsLoadHomePage").toBool() ? HomePage : BlankPage),
    m_activeView(nullptr),
    m_tabBar(new BrowserTabBar(this)),
    m_backMenu(nullptr),
    m_forwardMenu(nullptr),
    m_lastTabIndex(0),
    m_currentTabIndex(0),
    m_nextTabIndex(1),
    m_contextMenuPosGlobal(),
    m_contextMenuPosRelative(),
    m_mainWindow(qobject_cast<MainWindow*>(parent)),
    m_closedTabs()
{
    // Set tab bar
    setTabBar(m_tabBar);

    // Set tab widget UI properties
    setDocumentMode(true);
    setElideMode(Qt::ElideRight);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    connect(this, &BrowserTabWidget::tabCloseRequested, this, &BrowserTabWidget::closeTab);
    connect(this, &BrowserTabWidget::currentChanged, this, &BrowserTabWidget::onCurrentChanged);

    connect(m_tabBar, &BrowserTabBar::duplicateTabRequest, this, &BrowserTabWidget::duplicateTab);
    connect(m_tabBar, &BrowserTabBar::newTabRequest, [=](){ newTab(); });
    connect(m_tabBar, &BrowserTabBar::reloadTabRequest, [=](int index){
        if (WebView *view = getWebView(index))
            view->reload();
    });

    QCoreApplication::instance()->installEventFilter(this);
}

WebView *BrowserTabWidget::currentWebView() const
{
    return qobject_cast<WebView*>(currentWidget());
}

WebView *BrowserTabWidget::getWebView(int tabIndex) const
{
    if (QWidget *item = widget(tabIndex))
        return qobject_cast<WebView*>(item);
    return nullptr;
}

bool BrowserTabWidget::eventFilter(QObject *watched, QEvent *event)
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
                QTimer::singleShot(10, this, &BrowserTabWidget::showContextMenuForView);
                return true;
            }
            break;
        }
        case QEvent::MouseMove:
        {
            if (m_mainWindow && m_mainWindow->isFullScreen())
                m_mainWindow->onMouseMoveFullscreen(static_cast<QMouseEvent*>(event)->y());
            break;
        }
        case QEvent::KeyPress:
        {
            if (m_mainWindow && m_mainWindow->isFullScreen())
            {
                QKeyEvent *keyEvent = static_cast<QKeyEvent*>(event);
                if (keyEvent->key() == Qt::Key_F11)
                    m_mainWindow->onToggleFullScreen(false);
            }
            break;
        }
        default:
            break;
    }
    return QTabWidget::eventFilter(watched, event);
}

bool BrowserTabWidget::canReopenClosedTab() const
{
    return !m_closedTabs.empty();
}

void BrowserTabWidget::showContextMenuForView()
{
    currentWebView()->showContextMenu(m_contextMenuPosGlobal, m_contextMenuPosRelative);
}

void BrowserTabWidget::reopenLastTab()
{
    if (m_closedTabs.empty())
        return;

    auto &tabInfo = m_closedTabs.front();
    WebView *view = newTab(false, false, tabInfo.index);
    view->load(tabInfo.url);
    QDataStream historyStream(&tabInfo.pageHistory, QIODevice::ReadWrite);
    historyStream >> *(view->history());

    m_closedTabs.pop_front();
}

void BrowserTabWidget::saveTab(int index)
{
    WebView *view = getWebView(index);
    if (!view)
        return;

    ClosedTabInfo tabInfo(index, view);
    m_closedTabs.push_front(tabInfo);

    while (m_closedTabs.size() > 30)
        m_closedTabs.pop_back();
}

void BrowserTabWidget::closeTab(int index)
{
    int numTabs = count();
    if (index < 0 || numTabs == 1 || index >= numTabs)
        return;

    saveTab(index);
    WebView *view = getWebView(index);
    emit tabClosing(view);

    view->deleteLater();

    // If closed tab was the active tab, set current to opposite direction of the last active tab (if possible)
    if (index == m_currentTabIndex)
    {
        if (index == 0)
            setCurrentIndex(1);
        else if (index == count() - 1)
            setCurrentIndex(count() - 2);
        else
        {
            int nextIndex = (m_lastTabIndex > index ? index - 1 : index + 1);
            setCurrentIndex(nextIndex);
            if (nextIndex == index + 1)
            {
                m_lastTabIndex = index;
                m_currentTabIndex = index;
                m_nextTabIndex = nextIndex;
            }
        }
    }

    removeTab(index);
}

void BrowserTabWidget::duplicateTab(int index)
{
    if (WebView *view = getWebView(index))
        openLinkInNewTab(view->url(), false);
}

WebView *BrowserTabWidget::newTab(bool makeCurrent, bool skipHomePage, int specificIndex)
{
    WebView *view = new WebView(m_privateBrowsing, parentWidget());
    view->installEventFilter(this);

    QString tabLabel;
    if (!skipHomePage)
    {
        switch (m_newTabPage)
        {
            case HomePage:
                view->load(QUrl::fromUserInput(m_settings->getValue("HomePage").toString()));
                tabLabel = tr("Home Page");
                break;
            case BlankPage:
                view->loadBlankPage();
                tabLabel = tr("New Tab");
                break;
        }
    }

    // Connect web view signals to functionalty
    connect(view, &WebView::iconChanged,            this, &BrowserTabWidget::onIconChanged);
    connect(view, &WebView::loadFinished,           this, &BrowserTabWidget::resetHistoryButtonMenus);
    connect(view, &WebView::openRequest,            this, &BrowserTabWidget::loadUrl);
    connect(view, &WebView::openInNewTabRequest,    this, &BrowserTabWidget::openLinkInNewTab);
    connect(view, &WebView::openInNewWindowRequest, this, &BrowserTabWidget::openLinkInNewWindow);
    connect(view, &WebView::viewCloseRequested,     this, &BrowserTabWidget::onViewCloseRequested);
    connect(view, &WebView::fullScreenRequested, m_mainWindow, &MainWindow::onToggleFullScreen);
    connect(view, &WebView::loadProgress, [=](int progress){
        if (view == currentWebView())
            emit loadProgress(progress);
    });
    connect(view, &WebView::titleChanged, [=](const QString &title) {
        int viewTabIndex = indexOf(view);
        if (viewTabIndex >= 0)
        {
            setTabText(viewTabIndex, title);
            setTabToolTip(viewTabIndex, title);
        }
    });
    if (!m_privateBrowsing)
    {
        connect(view, &WebView::iconUrlChanged, [=](const QUrl &url) {
            sBrowserApplication->getFaviconStorage()->updateIcon(url.toString(QUrl::FullyEncoded), view->url().toString(), view->icon());
        });
    }

    if (specificIndex >= 0)
    {
        if (specificIndex > count())
            specificIndex = count();

        specificIndex = insertTab(specificIndex, view, tabLabel);
        if (specificIndex < m_nextTabIndex)
            ++m_nextTabIndex;
    }
    else
        m_nextTabIndex = insertTab(m_nextTabIndex, view, tabLabel) + 1;

    if (makeCurrent)
    {
        m_activeView = view;
        setCurrentWidget(view);
    }

    if (count() == 1)
    {
        m_activeView = view;
        emit currentChanged(currentIndex());
    }

    emit newTabCreated(view);
    return view;
}

void BrowserTabWidget::onIconChanged()
{
    WebView *view = qobject_cast<WebView*>(sender());
    int tabIndex = indexOf(view);
    if (tabIndex < 0 || !view)
        return;

    setTabIcon(tabIndex, sBrowserApplication->getFaviconStorage()->getFavicon(view->url()));
}


void BrowserTabWidget::openLinkInNewTab(const QUrl &url, bool makeCurrent)
{
    // Create view, load home page, add view to tab widget
    WebView *view = newTab(makeCurrent, true);
    view->load(url);
}

void BrowserTabWidget::openLinkInNewWindow(const QUrl &url, bool privateWindow)
{
    MainWindow *newWin = privateWindow ? sBrowserApplication->getNewPrivateWindow() : sBrowserApplication->getNewWindow();
    if (!newWin)
        return;
    newWin->loadUrl(url);
}

void BrowserTabWidget::loadUrl(const QUrl &url)
{
    currentWebView()->load(url);
}

void BrowserTabWidget::setNavHistoryMenus(QMenu *backMenu, QMenu *forwardMenu)
{
    if (!backMenu || !forwardMenu)
        return;

    m_backMenu = backMenu;
    m_forwardMenu = forwardMenu;
}

void BrowserTabWidget::resetZoomCurrentView()
{
    if (WebView *view = currentWebView())
        view->resetZoom();
}

void BrowserTabWidget::zoomInCurrentView()
{
    if (WebView *view = currentWebView())
        view->zoomIn();
}

void BrowserTabWidget::zoomOutCurrentView()
{
    if (WebView *view = currentWebView())
        view->zoomOut();
}

void BrowserTabWidget::onCurrentChanged(int index)
{
    WebView *view = getWebView(index);
    if (!view)
        return;

    m_activeView = view;

    resetHistoryButtonMenus(true);

    m_lastTabIndex = m_currentTabIndex;
    m_currentTabIndex = index;
    m_nextTabIndex = index + 1;

    m_tabBar->updateGeometry();

    emit loadProgress(view->getProgress());
    emit viewChanged(index);
}

void BrowserTabWidget::onViewCloseRequested()
{
    WebView *view = qobject_cast<WebView*>(sender());
    closeTab(indexOf(view));
}

void BrowserTabWidget::resetHistoryButtonMenus(bool /*ok*/)
{
    m_backMenu->clear();
    m_forwardMenu->clear();

    int maxMenuSize = 10;
    QWebEngineHistory *hist = m_activeView->page()->history();
    QAction *histAction = nullptr, *prevAction = nullptr;
    FaviconStorage *faviconStorage = sBrowserApplication->getFaviconStorage();

    // Setup back button history menu
    QList<QWebEngineHistoryItem> histItems = hist->backItems(maxMenuSize);
    for (const QWebEngineHistoryItem &item : histItems)
    {
        QIcon icon = faviconStorage->getFavicon(item.url());

        if (prevAction == nullptr)
            histAction = m_backMenu->addAction(icon, item.title());
        else
        {
            histAction = new QAction(icon, item.title(), m_backMenu);
            m_backMenu->insertAction(prevAction, histAction);
        }

        connect(histAction, &QAction::triggered, [=](){
            hist->goToItem(item);
        });
        prevAction = histAction;
    }

    // Setup forward button history menu
    histItems = hist->forwardItems(maxMenuSize);
    histAction = nullptr, prevAction = nullptr;
    for (const QWebEngineHistoryItem &item : histItems)
    {
        QIcon icon = faviconStorage->getFavicon(item.url());

        if (prevAction == nullptr)
            histAction = m_forwardMenu->addAction(icon, item.title());
        else
        {
            histAction = new QAction(icon, item.title(), m_forwardMenu);
            m_forwardMenu->insertAction(prevAction, histAction);
        }

        connect(histAction, &QAction::triggered, [=](){
            hist->goToItem(item);
        });
        prevAction = histAction;
    }
}
