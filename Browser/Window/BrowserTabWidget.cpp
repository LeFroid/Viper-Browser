#include "AdBlockManager.h"
#include "BrowserApplication.h"
#include "BrowserTabWidget.h"
#include "BrowserTabBar.h"
#include "FaviconStorage.h"
#include "MainWindow.h"
#include "WebPage.h"
#include "WebView.h"
#include "WebWidget.h"

#include <algorithm>
#include <QDesktopWidget>
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

BrowserTabWidget::BrowserTabWidget(std::shared_ptr<Settings> settings, FaviconStorage *faviconStore, bool privateMode, QWidget *parent) :
    QTabWidget(parent),
    m_settings(settings),
    m_faviconStore(faviconStore),
    m_privateBrowsing(privateMode),
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
    //setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    connect(this, &BrowserTabWidget::tabCloseRequested, this, &BrowserTabWidget::closeTab);
    connect(this, &BrowserTabWidget::currentChanged, this, &BrowserTabWidget::onCurrentChanged);

    connect(m_tabBar, &BrowserTabBar::duplicateTabRequest, this, &BrowserTabWidget::duplicateTab);
    connect(m_tabBar, &BrowserTabBar::newTabRequest, [=](){ newTab(); });
    connect(m_tabBar, &BrowserTabBar::reloadTabRequest, [=](int index){
        if (WebWidget *view = getWebWidget(index))
            view->reload();
    });

    QCoreApplication::instance()->installEventFilter(this);
}

WebWidget *BrowserTabWidget::currentWebWidget() const
{
    return qobject_cast<WebWidget*>(currentWidget());
}

WebWidget *BrowserTabWidget::getWebWidget(int tabIndex) const
{
    if (QWidget *item = widget(tabIndex))
        return qobject_cast<WebWidget*>(item);
    return nullptr;
}

bool BrowserTabWidget::eventFilter(QObject *watched, QEvent *event)
{
    switch (event->type())
    {
        case QEvent::MouseMove:
        {
            if (m_mainWindow && m_mainWindow->isFullScreen())
                m_mainWindow->onMouseMoveFullscreen(static_cast<QMouseEvent*>(event)->y());
            break;
        }

        case QEvent::Resize:
        {
            if (m_mainWindow != watched)
                break;

            QTimer::singleShot(50, [=](){
                const int screenWidth = sBrowserApplication->desktop()->screenGeometry().width();
                const QRect winGeom = m_mainWindow->geometry();
                if (winGeom.left() + m_mainWindow->width() > screenWidth)
                {
                    QSize winSize = m_mainWindow->size();
                    winSize.setWidth(screenWidth - winGeom.left());
                    m_mainWindow->resize(winSize);

                    setMaximumWidth(winSize.width());
                    currentWebWidget()->setMaximumWidth(winSize.width());
                }
            });

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
    return QObject::eventFilter(watched, event);
}

void BrowserTabWidget::setTabPinned(int index, bool value)
{
    m_tabBar->setTabPinned(index, value);
}

bool BrowserTabWidget::canReopenClosedTab() const
{
    return !m_closedTabs.empty();
}

bool BrowserTabWidget::isTabPinned(int tabIndex) const
{
    return m_tabBar->isTabPinned(tabIndex);
}

void BrowserTabWidget::reopenLastTab()
{
    if (m_closedTabs.empty())
        return;

    auto &tabInfo = m_closedTabs.front();
    WebWidget *ww = newTab(false, false, tabInfo.index);
    ww->load(tabInfo.url);
    QDataStream historyStream(&tabInfo.pageHistory, QIODevice::ReadWrite);
    historyStream >> *(ww->history());

    m_closedTabs.pop_front();
}

void BrowserTabWidget::saveTab(int index)
{
    WebWidget *ww = getWebWidget(index);
    if (!ww)
        return;

    ClosedTabInfo tabInfo(index, ww->view());
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
    WebWidget *ww = getWebWidget(index);
    emit tabClosing(ww);

    //ww->deleteLater();

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

    delete ww;
}

void BrowserTabWidget::closeCurrentTab()
{
    closeTab(currentIndex());
}

void BrowserTabWidget::duplicateTab(int index)
{
    if (WebWidget *ww = getWebWidget(index))
        openLinkInNewTab(ww->url(), false);
}

WebWidget *BrowserTabWidget::newTab(bool makeCurrent, bool skipHomePage, int specificIndex)
{
    WebWidget *ww = new WebWidget(m_privateBrowsing, this);
    ww->setupView();
    //WebView *view = new WebView(m_privateBrowsing, m_mainWindow);
    //view->installEventFilter(this);

    QString tabLabel;
    if (!skipHomePage)
    {
        auto newTabPage = m_settings->getValue(BrowserSetting::NewTabsLoadHomePage).toBool() ? HomePage : BlankPage;
        switch (newTabPage)
        {
            case HomePage:
                ww->load(QUrl::fromUserInput(m_settings->getValue(BrowserSetting::HomePage).toString()));
                tabLabel = tr("Home Page");
                break;
            case BlankPage:
                ww->loadBlankPage();
                tabLabel = tr("New Tab");
                break;
        }
    }

    // Connect web view signals to functionalty
    connect(ww, &WebWidget::iconChanged,            this, &BrowserTabWidget::onIconChanged);
    connect(ww, &WebWidget::loadFinished,           this, &BrowserTabWidget::resetHistoryButtonMenus);
    connect(ww, &WebWidget::loadProgress,           this, &BrowserTabWidget::onLoadProgress);
    connect(ww, &WebWidget::openRequest,            this, &BrowserTabWidget::loadUrl);
    connect(ww, &WebWidget::openInNewTabRequest,    this, &BrowserTabWidget::openLinkInNewTab);
    connect(ww, &WebWidget::openInNewWindowRequest, this, &BrowserTabWidget::openLinkInNewWindow);
    connect(ww, &WebWidget::titleChanged,           this, &BrowserTabWidget::onTitleChanged);
    connect(ww, &WebWidget::closeRequest,           this, &BrowserTabWidget::onViewCloseRequested);
    //connect(view, &WebWidget::fullScreenRequested, m_mainWindow, &MainWindow::onToggleFullScreen);

    //connect(view, &WebView::loadStarted, [view](){
    //    AdBlockManager::instance().loadStarted(view->url());
    //});

    if (!m_privateBrowsing)
    {
        connect(ww, &WebWidget::iconUrlChanged, [=](const QUrl &url) {
            m_faviconStore->updateIcon(url.toString(QUrl::FullyEncoded), ww->url().toString(), ww->getIcon());
        });
    }

    if (specificIndex >= 0)
    {
        if (specificIndex > count())
            specificIndex = count();

        specificIndex = insertTab(specificIndex, ww, tabLabel);
        if (specificIndex < m_nextTabIndex)
            ++m_nextTabIndex;
    }
    else
        m_nextTabIndex = insertTab(m_nextTabIndex, ww, tabLabel) + 1;

    if (makeCurrent)
    {
        m_activeView = ww;
        setCurrentWidget(ww);
    }
    else
    {
        ww->resize(currentWidget()->size());
        ww->show();
    }

    if (count() == 1)
    {
        m_activeView = ww;
        emit currentChanged(currentIndex());
    }

    emit newTabCreated(ww);
    return ww;
}

void BrowserTabWidget::onIconChanged()
{
    WebWidget *ww = qobject_cast<WebWidget*>(sender());
    int tabIndex = indexOf(ww);
    if (tabIndex < 0 || !ww)
        return;

    QIcon icon = ww->getIcon();
    if (icon.isNull())
        icon = m_faviconStore->getFavicon(ww->url());

    setTabIcon(tabIndex, icon);
}


void BrowserTabWidget::openLinkInNewTab(const QUrl &url, bool makeCurrent)
{
    // Create view, load home page, add view to tab widget
    WebWidget *ww = newTab(makeCurrent, true);
    ww->load(url);
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
    currentWebWidget()->load(url);
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
    if (WebWidget *ww = currentWebWidget())
        ww->view()->resetZoom();
}

void BrowserTabWidget::zoomInCurrentView()
{
    if (WebWidget *ww = currentWebWidget())
        ww->view()->zoomIn();
}

void BrowserTabWidget::zoomOutCurrentView()
{
    if (WebWidget *ww = currentWebWidget())
        ww->view()->zoomOut();
}

void BrowserTabWidget::onCurrentChanged(int index)
{
    WebWidget *ww = getWebWidget(index);
    if (!ww)
        return;

    m_activeView = ww;

    resetHistoryButtonMenus(true);

    m_lastTabIndex = m_currentTabIndex;
    m_currentTabIndex = index;
    m_nextTabIndex = index + 1;

    m_tabBar->updateGeometry();

    emit loadProgress(ww->getProgress());
    emit viewChanged(index);
}

void BrowserTabWidget::onLoadProgress(int progress)
{
    WebWidget *ww = qobject_cast<WebWidget*>(sender());
    if (ww == currentWebWidget())
        emit loadProgress(progress);
}

void BrowserTabWidget::onTitleChanged(const QString &title)
{
    WebWidget *ww = qobject_cast<WebWidget*>(sender());
    int viewTabIndex = indexOf(ww);
    if (viewTabIndex >= 0)
    {
        setTabText(viewTabIndex, title);
        setTabToolTip(viewTabIndex, title);
    }
}

void BrowserTabWidget::onViewCloseRequested()
{
    WebWidget *ww = qobject_cast<WebWidget*>(sender());
    closeTab(indexOf(ww));
}

void BrowserTabWidget::resetHistoryButtonMenus(bool /*ok*/)
{
    m_backMenu->clear();
    m_forwardMenu->clear();

    int maxMenuSize = 10;
    QWebEngineHistory *hist = m_activeView->history();
    QAction *histAction = nullptr, *prevAction = nullptr;

    // Setup back button history menu
    QList<QWebEngineHistoryItem> histItems = hist->backItems(maxMenuSize);
    for (const QWebEngineHistoryItem &item : histItems)
    {
        QIcon icon = m_faviconStore->getFavicon(item.url());

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
        QIcon icon = m_faviconStore->getFavicon(item.url());

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
