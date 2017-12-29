#include "BrowserApplication.h"
#include "BrowserTabWidget.h"
#include "MainWindow.h"
#include "ui_mainwindow.h"
#include "AdBlockWidget.h"
#include "AddBookmarkDialog.h"
#include "BookmarkBar.h"
#include "BookmarkNode.h"
#include "BookmarkWidget.h"
#include "CodeEditor.h"
#include "CookieWidget.h"
#include "DownloadManager.h"
#include "FaviconStorage.h"
#include "ClearHistoryDialog.h"
#include "HistoryManager.h"
#include "HistoryWidget.h"
#include "HTMLHighlighter.h"
#include "Preferences.h"
#include "SecurityManager.h"
#include "SearchEngineLineEdit.h"
#include "URLLineEdit.h"
#include "URLSuggestionModel.h"
#include "UserAgentManager.h"
#include "UserScriptManager.h"
#include "UserScriptWidget.h"
#include "WebPage.h"
#include "WebView.h"

#include <deque>
#include <functional>
#include <QActionGroup>
#include <QCloseEvent>
#include <QDir>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QFileDialog>
#include <QKeySequence>
#include <QMessageBox>
#include <QMimeData>
#include <QPrinter>
#include <QPrintPreviewDialog>
#include <QPushButton>
#include <QShortcut>
#include <QSplitter>
#include <QTimer>
#include <QToolButton>
#include <QWebElement>
#include <QWebFrame>
#include <QWebInspector>

MainWindow::MainWindow(std::shared_ptr<Settings> settings, BookmarkManager *bookmarkManager, QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    m_privateWindow(false),
    m_settings(settings),
    m_bookmarkManager(bookmarkManager),
    m_bookmarkUI(nullptr),
    m_clearHistoryDialog(nullptr),
    m_cookieUI(nullptr),
    m_tabWidget(nullptr),
    m_addPageBookmarks(nullptr),
    m_removePageBookmarks(nullptr),
    m_userAgentGroup(new QActionGroup(this)),
    m_preferences(nullptr),
    m_addBookmarkDialog(nullptr),
    m_userScriptWidget(nullptr),
    m_adBlockWidget(nullptr)
{
    setAttribute(Qt::WA_DeleteOnClose, true);
    setToolButtonStyle(Qt::ToolButtonFollowStyle);
    setAcceptDrops(true);

    ui->setupUi(this);

    setupToolBar();
    setupTabWidget();
    setupBookmarks();
    setupMenuBar();

    ui->dockWidget->hide();
    ui->widgetFindText->hide();
    m_tabWidget->currentWebView()->setFocus();
}

MainWindow::~MainWindow()
{
    delete ui;

    for (WebActionProxy *proxy : m_webActions)
        delete proxy;

    if (m_bookmarkUI)
        delete m_bookmarkUI;

    if (m_cookieUI)
        delete m_cookieUI;

    if (m_preferences)
        delete m_preferences;

    if (m_addBookmarkDialog)
        delete m_addBookmarkDialog;

    if (m_userScriptWidget)
        delete m_userScriptWidget;
}

bool MainWindow::isPrivate() const
{
    return m_privateWindow;
}

void MainWindow::setPrivate(bool value)
{
    m_privateWindow = value;
    m_tabWidget->setPrivateMode(value);

    if (m_privateWindow)
        setWindowTitle("Web Browser - Private Browsing");
}

void MainWindow::addHistoryItem(const QUrl &url, const QString &title, const QIcon &favicon)
{
    QMenu *menu = ui->menuHistory;
    QList<QAction*> menuActions = menu->actions();

    QAction *beforeItem = nullptr;
    if (menuActions.size() > 3)
        beforeItem = menuActions[3];

    QAction *historyItem = new QAction(title, menu);
    historyItem->setIcon(favicon);
    connect(historyItem, &QAction::triggered, [=](){
        m_tabWidget->loadUrl(url);
    });

    if (beforeItem)
        menu->insertAction(beforeItem, historyItem);
    else
        menu->addAction(historyItem);

    int menuSize = menuActions.size();
    QAction *actionToRemove = nullptr;
    while (menuSize > 17)
    {
        actionToRemove = menuActions[menuSize - 1];
        menu->removeAction(actionToRemove);
        --menuSize;
    }
}

void MainWindow::clearHistoryItems()
{
    QList<QAction*> menuActions = ui->menuHistory->actions();
    if (menuActions.size() < 3)
        return;

    QAction *currItem = nullptr;
    for (int i = 3; i < menuActions.size(); ++i)
    {
        currItem = menuActions.at(i);
        ui->menuHistory->removeAction(currItem);
    }
}

void MainWindow::resetUserAgentMenu()
{
    // First remove old items, then repopulate the menu
    ui->menuUser_Agents->clear();
    UserAgentManager *uaManager = sBrowserApplication->getUserAgentManager();
    QAction *currItem = ui->menuUser_Agents->addAction(tr("Default"), uaManager, &UserAgentManager::disableActiveAgent);
    currItem->setCheckable(true);
    m_userAgentGroup->addAction(currItem);
    currItem->setChecked(true);

    QMenu *subMenu = nullptr;
    for (auto it = uaManager->getAgentIterBegin(); it != uaManager->getAgentIterEnd(); ++it)
    {
        subMenu = new QMenu(it.key(), this);
        ui->menuUser_Agents->addMenu(subMenu);

        bool hasActiveAgentInMenu = (m_settings->getValue("CustomUserAgent").toBool()
                && (uaManager->getUserAgentCategory().compare(it.key()) == 0));

        for (auto agentIt = it->cbegin(); agentIt != it->cend(); ++agentIt)
        {
            QAction *action = subMenu->addAction(agentIt->Name);
            action->setData(agentIt->Value);
            action->setCheckable(true);
            m_userAgentGroup->addAction(action);
            if (hasActiveAgentInMenu && (agentIt->Name.compare(uaManager->getUserAgent().Name) == 0))
                action->setChecked(true);
        }

        connect(subMenu, &QMenu::triggered, [=](QAction *action){
            UserAgent agent;
            agent.Name = action->text();
            // Remove first character from UA name, as it begins with a '&'
            agent.Name = agent.Name.right(agent.Name.size() - 1);
            agent.Value = action->data().toString();
            uaManager->setActiveAgent(it.key(), agent);
            action->setChecked(true);
        });
    }

    ui->menuUser_Agents->addSeparator();
    ui->menuUser_Agents->addAction(tr("Add User Agent"), uaManager, &UserAgentManager::addUserAgent);
    ui->menuUser_Agents->addAction(tr("Modify User Agents"), uaManager, &UserAgentManager::modifyUserAgents);
}

void MainWindow::loadBlankPage()
{
    m_tabWidget->currentWebView()->loadBlankPage();
}

void MainWindow::loadUrl(const QUrl &url)
{
    m_tabWidget->loadUrl(url);
}

void MainWindow::openLinkNewTab(const QUrl &url)
{
    m_tabWidget->openLinkInNewTab(url);
}

void MainWindow::openLinkNewWindow(const QUrl &url)
{
    m_tabWidget->openLinkInNewWindow(url, m_privateWindow);
}

void MainWindow::setupBookmarks()
{
    ui->menuBookmarks->addAction(tr("Manage Bookmarks"), this, SLOT(openBookmarkWidget()));
    ui->menuBookmarks->addSeparator();
    setupBookmarkFolder(m_bookmarkManager->getRoot(), ui->menuBookmarks);

    m_addPageBookmarks = new QAction(tr("Bookmark this page"), this);
    connect(m_addPageBookmarks, &QAction::triggered, this, &MainWindow::addPageToBookmarks);

    m_removePageBookmarks = new QAction(tr("Remove this bookmark"), this);
    connect(m_removePageBookmarks, &QAction::triggered, this, &MainWindow::removePageFromBookmarks);

    // Setup bookmark bar
    ui->bookmarkBar->setBookmarkManager(m_bookmarkManager);
    connect(ui->bookmarkBar, &BookmarkBar::loadBookmark, m_tabWidget, &BrowserTabWidget::loadUrl);
    connect(ui->bookmarkBar, &BookmarkBar::loadBookmarkNewTab, m_tabWidget, &BrowserTabWidget::openLinkInNewTab);
    connect(ui->bookmarkBar, &BookmarkBar::loadBookmarkNewWindow, [=](const QUrl &url){
        m_tabWidget->openLinkInNewWindow(url, m_privateWindow);
    });
}

void MainWindow::setupBookmarkFolder(BookmarkNode *folder, QMenu *folderMenu)
{
    if (!folderMenu)
        return;

    FaviconStorage *iconStorage = sBrowserApplication->getFaviconStorage();

    std::deque< std::pair<BookmarkNode*, QMenu*> > folders;
    folders.push_back({folder, folderMenu});

    BookmarkNode *currentNode;
    QMenu *currentMenu;

    while (!folders.empty())
    {
        auto &current = folders.front();
        currentNode = current.first;
        currentMenu = current.second;

        int numChildren = currentNode->getNumChildren();
        for (int i = 0; i < numChildren; ++i)
        {
            BookmarkNode *n = currentNode->getNode(i);
            if (n->getType() == BookmarkNode::Folder)
            {
                QMenu *subMenu = currentMenu->addMenu(n->getIcon(), n->getName());
                folders.push_back({n, subMenu});
                continue;
            }

            QUrl link(n->getURL());
            QAction *item = currentMenu->addAction(iconStorage->getFavicon(link), n->getName());
            item->setIconVisibleInMenu(true);
            currentMenu->addAction(item);
            connect(item, &QAction::triggered, [=](){ loadUrl(link); });
        }

        folders.pop_front();
    }
}

void MainWindow::setupMenuBar()
{
    // File menu slots
    connect(ui->actionNew_Tab, &QAction::triggered, [=](bool){
        m_tabWidget->newTab();
    });
    connect(ui->actionNew_Window, &QAction::triggered, sBrowserApplication, &BrowserApplication::getNewWindow);
    connect(ui->actionNew_Private_Window, &QAction::triggered, sBrowserApplication, &BrowserApplication::getNewPrivateWindow);
    connect(ui->actionClose_Tab, &QAction::triggered, [=](){
        m_tabWidget->closeTab(m_tabWidget->currentIndex());
    });
    connect(ui->actionClose_Window, &QAction::triggered, this, &MainWindow::close);
    connect(ui->action_Quit, &QAction::triggered, sBrowserApplication, &BrowserApplication::quit);
    connect(ui->actionOpen_File, &QAction::triggered, this, &MainWindow::openFileInBrowser);
    connect(ui->action_Print, &QAction::triggered, this, &MainWindow::printTabContents);

    // Find action
    connect(ui->action_Find, &QAction::triggered, this, &MainWindow::onFindTextAction);

    // Add proxy functionality to edit menu actions
    addWebProxyAction(QWebPage::Undo, ui->action_Undo);
    addWebProxyAction(QWebPage::Redo, ui->action_Redo);
    addWebProxyAction(QWebPage::Cut, ui->actionCu_t);
    addWebProxyAction(QWebPage::Copy, ui->action_Copy);
    addWebProxyAction(QWebPage::Paste, ui->action_Paste);

    // Add proxy for reload action in menu bar
    addWebProxyAction(QWebPage::Reload, ui->actionReload);

    // Zoom in / out / reset slots
    connect(ui->actionZoom_In, &QAction::triggered, m_tabWidget, &BrowserTabWidget::zoomInCurrentView);
    connect(ui->actionZoom_Out, &QAction::triggered, m_tabWidget, &BrowserTabWidget::zoomOutCurrentView);
    connect(ui->actionReset_Zoom, &QAction::triggered, m_tabWidget, &BrowserTabWidget::resetZoomCurrentView);

    // Full screen view mode
    connect(ui->action_Full_Screen, &QAction::triggered, this, &MainWindow::onToggleFullScreen);

    // Preferences window
    connect(ui->actionPreferences, &QAction::triggered, [=](){
        if (!m_preferences)
            m_preferences = new Preferences(m_settings);

        m_preferences->loadSettings();
        m_preferences->show();
    });

    // View-related
    connect(ui->actionPage_So_urce, &QAction::triggered, this, &MainWindow::onRequestViewSource);

    // Bookmark bar setting
    ui->actionBookmark_Bar->setChecked(m_settings->getValue("EnableBookmarkBar").toBool());
    connect(ui->actionBookmark_Bar, &QAction::toggled, this, &MainWindow::toggleBookmarkBar);
    toggleBookmarkBar(ui->actionBookmark_Bar->isChecked());

    // History menu items
    connect(ui->actionShow_all_history, &QAction::triggered, this, &MainWindow::onShowAllHistory);
    connect(ui->actionClear_Recent_History, &QAction::triggered, [=]() {
        if (!m_clearHistoryDialog)
        {
            m_clearHistoryDialog = new ClearHistoryDialog(this);
            connect(m_clearHistoryDialog, &ClearHistoryDialog::finished, this, &MainWindow::onClearHistoryDialogFinished);
        }
        m_clearHistoryDialog->show();
    });

    // Tools menu
    connect(ui->actionManage_Ad_Blocker, &QAction::triggered, this, &MainWindow::openAdBlockManager);
    connect(ui->actionManage_Cookies,    &QAction::triggered, this, &MainWindow::openCookieManager);
    connect(ui->actionUser_Scripts,      &QAction::triggered, this, &MainWindow::openUserScriptManager);
    connect(ui->actionView_Downloads,    &QAction::triggered, this, &MainWindow::openDownloadManager);

    // User agent sub-menu
    resetUserAgentMenu();

    // Help menu
    connect(ui->actionAbout, &QAction::triggered, [=](){
        QString appName = sBrowserApplication->applicationName();
        QString appVersion = sBrowserApplication->applicationVersion();
        QMessageBox::about(this, QString("About %1").arg(appName), QString("%1 - Version %2\nDeveloped by Timothy Vaccarelli").arg(appName).arg(appVersion));
    });
    connect(ui->actionAbout_Qt, &QAction::triggered, [=](){
        QMessageBox::aboutQt(this, tr("About Qt"));
    });

    // Set web page for proxy actions (called automatically during onTabChanged event after all UI elements are set up)
    if (WebView *view = m_tabWidget->getWebView(0))
    {
        QWebPage *page = view->page();
        for (WebActionProxy *proxy : m_webActions)
            proxy->setPage(page);
    }
}

void MainWindow::setupTabWidget()
{
    // Create tab widget and insert into the layout
    m_tabWidget = new BrowserTabWidget(m_settings, this);
    ui->verticalLayout->insertWidget(ui->verticalLayout->indexOf(ui->widgetFindText), m_tabWidget);

    // Add change tab slot after removing dummy tabs to avoid segfaulting
    connect(m_tabWidget, &BrowserTabWidget::viewChanged, this, &MainWindow::onTabChanged);

    // Some singals emitted by WebViews (which are managed largely by the tab widget) must be dealt with in the MainWindow
    connect(m_tabWidget, &BrowserTabWidget::newTabCreated, this, &MainWindow::onNewTabCreated);

    // Page load progress handler
    connect(m_tabWidget, &BrowserTabWidget::loadProgress, this, &MainWindow::onLoadProgress);

    // Set back/forward button history menus
    m_tabWidget->setNavHistoryMenus(m_prevPage->menu(), m_nextPage->menu());

    // Add first tab
    m_tabWidget->newTab(true);
}

void MainWindow::setupToolBar()
{
    ui->toolBar->setStyleSheet("QToolBar { spacing: 3px; } QToolButton::menu-indicator { subcontrol-position: bottom right; subcontrol-origin: content; }");

    // Previous Page Button
    m_prevPage = new QToolButton(ui->toolBar);
    QAction *prevPageAction = ui->toolBar->addWidget(m_prevPage);
    m_prevPage->setIcon(style()->standardIcon(QStyle::SP_ArrowBack, 0, this));
    m_prevPage->setToolTip(tr("Go back one page"));
    QMenu *buttonHistMenu = new QMenu(this);
    m_prevPage->setMenu(buttonHistMenu);
    connect(m_prevPage, &QToolButton::clicked, prevPageAction, &QAction::trigger);
    addWebProxyAction(QWebPage::Back, prevPageAction);

    // Next Page Button
    m_nextPage = new QToolButton(ui->toolBar);
    QAction *nextPageAction = ui->toolBar->addWidget(m_nextPage);
    m_nextPage->setIcon(style()->standardIcon(QStyle::SP_ArrowForward, 0, this));
    m_nextPage->setToolTip(tr("Go forward one page"));
    buttonHistMenu = new QMenu(this);
    m_nextPage->setMenu(buttonHistMenu);
    connect(m_nextPage, &QToolButton::clicked, nextPageAction, &QAction::trigger);
    addWebProxyAction(QWebPage::Forward, nextPageAction);

    // Stop Loading / Refresh Page dual button
    m_stopRefresh = new QAction(this);
    m_stopRefresh->setIcon(style()->standardIcon(QStyle::SP_BrowserReload));
    connect(m_stopRefresh, &QAction::triggered, [=](){
        if (WebView *view = m_tabWidget->currentWebView())
            view->reload();
    });

    // URL Bar
    m_urlInput = new URLLineEdit(this);
    connect(m_urlInput, &URLLineEdit::returnPressed, this, &MainWindow::goToURL);
    connect(m_urlInput, &URLLineEdit::viewSecurityInfo, this, &MainWindow::onClickSecurityInfo);

    // Quick search tool
    m_searchEngineLineEdit = new SearchEngineLineEdit(this);
    connect(m_searchEngineLineEdit, &SearchEngineLineEdit::requestPageLoad, this, &MainWindow::loadUrl);

    // Splitter for resizing URL bar and quick search bar
    QSplitter *splitter = new QSplitter(ui->toolBar);
    splitter->addWidget(m_urlInput);
    splitter->addWidget(m_searchEngineLineEdit);

    // Reduce height of URL bar and quick search
    int lineEditHeight = ui->toolBar->height() * 2 / 3 + 1;
    m_urlInput->setMaximumHeight(lineEditHeight);
    m_searchEngineLineEdit->setMaximumHeight(lineEditHeight);

    // Set width of URL bar to be larger than quick search
    QList<int> splitterSizes;
    int totalWidth = splitter->size().width();
    splitterSizes.push_back(totalWidth * 3 / 4);
    splitterSizes.push_back(totalWidth / 4);
    splitter->setSizes(splitterSizes);
    splitter->setStretchFactor(0, 1);
    splitter->setStretchFactor(1, 0);

    ui->toolBar->addAction(m_stopRefresh);
    ui->toolBar->addWidget(splitter);
}

void MainWindow::checkPageForBookmark()
{
    WebView *view = m_tabWidget->currentWebView();
    if (!view)
        return;

    ui->menuBookmarks->removeAction(m_addPageBookmarks);
    ui->menuBookmarks->removeAction(m_removePageBookmarks);
    QList<QAction*> menuActions = ui->menuBookmarks->actions();
    if (menuActions.empty())
        return;
    int lastActionPos = (menuActions.size() > 1) ? 1 : 0;
    QAction *firstBookmark = menuActions[lastActionPos];
    QAction *actionToAdd = (m_bookmarkManager->isBookmarked(view->url().toString())) ? m_removePageBookmarks : m_addPageBookmarks;
    ui->menuBookmarks->insertAction(firstBookmark, actionToAdd);
}

void MainWindow::onTabChanged(int index)
{
    WebView *view = m_tabWidget->getWebView(index);
    if (!view)
        return;

    // Update UI elements to reflect current view
    ui->widgetFindText->setWebView(view);
    ui->widgetFindText->hide();
    m_urlInput->setURL(view->url());

    if (ui->dockWidget->isVisible())
        view->inspectElement();

    checkPageForBookmark();

    // Change current page for web proxies
    QWebPage *page = view->page();
    for (WebActionProxy *proxy : m_webActions)
        proxy->setPage(page);

    // Give focus to the url line edit widget when changing to a blank tab
    if (m_urlInput->text().isEmpty())
        m_urlInput->setFocus();
}

void MainWindow::openBookmarkWidget()
{
    if (!m_bookmarkUI)
    {
        // Setup bookmark manager UI
        m_bookmarkUI = new BookmarkWidget;
        m_bookmarkUI->setBookmarkManager(m_bookmarkManager);
        connect(m_bookmarkUI, &BookmarkWidget::managerClosed, sBrowserApplication, &BrowserApplication::updateBookmarkMenus);
        connect(m_bookmarkUI, &BookmarkWidget::openBookmark, m_tabWidget, &BrowserTabWidget::loadUrl);
        connect(m_bookmarkUI, &BookmarkWidget::openBookmarkNewTab, m_tabWidget, &BrowserTabWidget::openLinkInNewTab);
        connect(m_bookmarkUI, &BookmarkWidget::openBookmarkNewWindow, this, &MainWindow::openLinkNewWindow);
    }
    m_bookmarkUI->show();
}

void MainWindow::updateTabIcon(QIcon icon, int tabIndex)
{
    m_tabWidget->setTabIcon(tabIndex, icon);
}

void MainWindow::updateTabTitle(const QString &title, int tabIndex)
{
     m_tabWidget->setTabText(tabIndex, title);
     m_tabWidget->setTabToolTip(tabIndex, title);
}

void MainWindow::goToURL()
{
    WebView *view = m_tabWidget->currentWebView();
    if (view)
    {
        QUrl location = QUrl::fromUserInput(m_urlInput->text());
        if (location.isValid())
        {
            view->load(location);
            m_urlInput->setText(location.toString(QUrl::FullyEncoded));
        }
    }
}

void MainWindow::openCookieManager()
{
    if (!m_cookieUI)
    {
        m_cookieUI = new CookieWidget;
        connect(m_cookieUI, &CookieWidget::destroyed, [=]() {
            this->m_cookieUI = nullptr;
        });
    }
    m_cookieUI->show();
}

void MainWindow::openDownloadManager()
{
    DownloadManager *mgr = sBrowserApplication->getDownloadManager();
    if (mgr->isHidden())
        mgr->show();
}

void MainWindow::onClearHistoryDialogFinished(int result)
{
    if (result == QDialog::Rejected)
        return;

    // Create a QDateTime object representing the start time to delete history
    qint64 hourInSecond = 3600;
    QDateTime now = QDateTime::currentDateTime();
    QDateTime timeRange;
    switch (m_clearHistoryDialog->getTimeRange())
    {
        case ClearHistoryDialog::LAST_HOUR:
            timeRange = now.addSecs(-hourInSecond);
            break;
        case ClearHistoryDialog::LAST_TWO_HOUR:
            timeRange = now.addSecs(-2 * hourInSecond);
            break;
        case ClearHistoryDialog::LAST_FOUR_HOUR:
            timeRange = now.addSecs(-4 * hourInSecond);
            break;
        case ClearHistoryDialog::LAST_DAY:
            timeRange = now.addSecs(-24 * hourInSecond);
            break;
        default:
            break;
    }

    // Pass start time and history deletion types to BrowserApplication
    sBrowserApplication->clearHistory(m_clearHistoryDialog->getHistoryTypes(), timeRange);
}

void MainWindow::refreshBookmarkMenu()
{
    ui->menuBookmarks->clear();
    ui->menuBookmarks->addAction(tr("Manage Bookmarks"), this, SLOT(openBookmarkWidget()));
    ui->menuBookmarks->addSeparator();
    setupBookmarkFolder(m_bookmarkManager->getRoot(), ui->menuBookmarks);
    checkPageForBookmark();

    // Bookmark bar
    ui->bookmarkBar->refresh();

    // Bookmark widget
    if (m_bookmarkUI)
        m_bookmarkUI->reloadBookmarks();
}

void MainWindow::addPageToBookmarks()
{
    WebView *view = m_tabWidget->currentWebView();
    if (!view)
        return;

    const QString bookmarkName = view->title();
    const QString bookmarkUrl = view->url().toString();
    if (m_bookmarkManager->isBookmarked(bookmarkUrl))
        return;
    m_bookmarkManager->appendBookmark(bookmarkName, bookmarkUrl);

    if (!m_addBookmarkDialog)
    {
        m_addBookmarkDialog = new AddBookmarkDialog(m_bookmarkManager);
        connect(m_addBookmarkDialog, &AddBookmarkDialog::updateBookmarkMenu, sBrowserApplication, &BrowserApplication::updateBookmarkMenus);
    }
    m_addBookmarkDialog->setBookmarkInfo(bookmarkName, bookmarkUrl);

    // Set position of add bookmark dialog to align just under the URL bar on the right side
    QPoint dialogPos;
    const QRect winGeom = frameGeometry();
    const QRect toolbarGeom = ui->toolBar->frameGeometry();
    const QRect urlBarGeom = m_urlInput->frameGeometry();
    dialogPos.setX(winGeom.x() + toolbarGeom.x() + urlBarGeom.x()
                   + urlBarGeom.width() - m_addBookmarkDialog->width());
    dialogPos.setY(winGeom.y() + toolbarGeom.y() + toolbarGeom.height() + (urlBarGeom.height() * 2 / 3));
    m_addBookmarkDialog->move(dialogPos);

    m_addBookmarkDialog->show();
}

void MainWindow::removePageFromBookmarks()
{
    WebView *view = m_tabWidget->currentWebView();
    if (!view)
        return;

    m_bookmarkManager->removeBookmark(view->url().toString());
    QMessageBox::information(this, tr("Bookmark"), tr("Page removed from bookmarks."));
    sBrowserApplication->updateBookmarkMenus();
}

void MainWindow::toggleBookmarkBar(bool enabled)
{
    if (enabled)
        ui->bookmarkBar->show();
    else
        ui->bookmarkBar->hide();

    m_settings->setValue("EnableBookmarkBar", enabled);
}

void MainWindow::onFindTextAction()
{
    ui->widgetFindText->show();
    auto lineEdit = ui->widgetFindText->getLineEdit();
    lineEdit->setFocus();
    lineEdit->selectAll();
}

void MainWindow::openAdBlockManager()
{
    if (!m_adBlockWidget)
    {
        m_adBlockWidget = new AdBlockWidget;
        connect(m_adBlockWidget, &AdBlockWidget::destroyed, [=](){
            m_adBlockWidget = nullptr;
        });
    }
    m_adBlockWidget->show();
    m_adBlockWidget->raise();
    m_adBlockWidget->activateWindow();
}

void MainWindow::openUserScriptManager()
{
    if (!m_userScriptWidget)
    {
        m_userScriptWidget = new UserScriptWidget;
        connect(m_userScriptWidget, &UserScriptWidget::destroyed, [=](){
            m_userScriptWidget = nullptr;
        });
    }
    m_userScriptWidget->show();
    m_userScriptWidget->raise();
    m_userScriptWidget->activateWindow();
}

void MainWindow::openFileInBrowser()
{
    QString fileName = QFileDialog::getOpenFileName(this, tr("Open File"), QDir::homePath());
    if (!fileName.isEmpty())
        loadUrl(QUrl(QString("file://%1").arg(fileName)));
}

void MainWindow::onLoadProgress(int value)
{
    // Update stop/refresh icon and tooltip depending on state
    if (value > 0 && value < 100)
    {
        m_stopRefresh->setIcon(style()->standardIcon(QStyle::SP_BrowserStop));
        m_stopRefresh->setToolTip(tr("Stop loading the page"));
    }
    else
    {
        m_stopRefresh->setIcon(style()->standardIcon(QStyle::SP_BrowserReload));
        m_stopRefresh->setToolTip(tr("Reload the page"));
    }
}

void MainWindow::onLoadFinished(WebView *view, bool /*ok*/)
{
    if (!view)
        return;

    const QString pageTitle = view->title();
    updateTabIcon(QWebSettings::iconForUrl(view->url()), m_tabWidget->indexOf(view));
    updateTabTitle(pageTitle, m_tabWidget->indexOf(view));

    if (m_tabWidget->currentWebView() == view)
    {
        m_urlInput->setURL(view->url());
        checkPageForBookmark();
    }

    // Get favicon and inform HistoryManager of the title and favicon associated with the page's url, if not in private mode
    if (!m_privateWindow)
    {
        BrowserApplication *browserApp = sBrowserApplication;

        QString pageUrl = view->url().toString();
        QWebElement faviconElement = view->page()->mainFrame()->findFirstElement("link[rel*='icon']");
        if (!faviconElement.isNull())
        {
            QString iconRef = faviconElement.attribute("href");
            if (!iconRef.isNull())
            {
                if (iconRef.startsWith('/'))
                    iconRef.prepend(view->url().toString(QUrl::RemovePath));
                browserApp->getFaviconStorage()->updateIcon(iconRef, pageUrl);
            }
        }
        if (!pageTitle.isEmpty())
            browserApp->getHistoryManager()->setTitleForURL(pageUrl, pageTitle);
    }
}

void MainWindow::onShowAllHistory()
{
    HistoryWidget *histWidget = sBrowserApplication->getHistoryWidget();
    histWidget->loadHistory();

    // Attach signals to current browser window
    disconnect(histWidget, &HistoryWidget::openLink, m_tabWidget, &BrowserTabWidget::loadUrl);
    disconnect(histWidget, &HistoryWidget::openLinkNewTab, m_tabWidget, &BrowserTabWidget::openLinkInNewTab);
    disconnect(histWidget, &HistoryWidget::openLinkNewWindow, this, &MainWindow::openLinkNewWindow);

    connect(histWidget, &HistoryWidget::openLink, m_tabWidget, &BrowserTabWidget::loadUrl);
    connect(histWidget, &HistoryWidget::openLinkNewTab, m_tabWidget, &BrowserTabWidget::openLinkInNewTab);
    connect(histWidget, &HistoryWidget::openLinkNewWindow, this, &MainWindow::openLinkNewWindow);

    histWidget->show();
}

void MainWindow::onNewTabCreated(WebView *view)
{
    // Connect signals to slots for UI updates (page title, icon changes)
    connect(view, &WebView::loadFinished, [=](bool ok) {
        onLoadFinished(view, ok);
    });
    connect(view, &WebView::titleChanged, [=](const QString &title){ updateTabTitle(title, m_tabWidget->indexOf(view));} );
    connect(view, &WebView::inspectElement, [=]() {
        QWebInspector *inspector = new QWebInspector(ui->dockWidget);
        inspector->setPage(view->page());
        ui->dockWidget->setWidget(inspector);
        ui->dockWidget->show();
    });
}

void MainWindow::onClickSecurityInfo()
{
    WebView *currentView = m_tabWidget->currentWebView();
    if (!currentView)
        return;
    SecurityManager::instance().showSecurityInfo(currentView->url().host());
}

void MainWindow::onRequestViewSource()
{
    WebView *currentView = m_tabWidget->currentWebView();
    if (!currentView)
        return;

    QString pageSource = currentView->page()->mainFrame()->toHtml();
    QString pageTitle = currentView->title();
    CodeEditor *view = new CodeEditor;
    view->setPlainText(pageSource);
    HTMLHighlighter *h = new HTMLHighlighter;
    h->setDocument(view->document());
    view->setReadOnly(true);
    view->setWindowTitle(tr("Viewing Source of %1").arg(pageTitle));
    view->setMinimumWidth(640);
    view->setMinimumHeight(geometry().height() / 2);
    view->setAttribute(Qt::WA_DeleteOnClose);
    view->show();
}

void MainWindow::onToggleFullScreen(bool enable)
{
    if (enable)
        showFullScreen();
    else
    {
        auto state = windowState();
        if (state & Qt::WindowMinimized)
            showMinimized();
        else if (state & Qt::WindowMaximized)
            showMaximized();
        else
            showNormal();
    }
}

void MainWindow::printTabContents()
{
    WebView *currentView = m_tabWidget->currentWebView();
    if (!currentView)
        return;

    QPrinter printer(QPrinter::HighResolution);
    printer.setPaperSize(QPrinter::Letter);
    printer.setFullPage(true);
    QPrintPreviewDialog dialog(&printer, this);
    dialog.setWindowTitle(tr("Print Document"));
    connect(&dialog, &QPrintPreviewDialog::paintRequested, currentView->page()->mainFrame(), &QWebFrame::print);
    dialog.exec();
}

WebView *MainWindow::getNewTabWebView()
{
    return m_tabWidget->newTab();
}

BrowserTabWidget *MainWindow::getTabWidget() const
{
    return m_tabWidget;
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if (!m_privateWindow)
    {
        StartupMode mode = m_settings->getValue("StartupMode").value<StartupMode>();
        if (mode == StartupMode::RestoreSession)
        {
            emit aboutToClose();
        }
    }

    QMainWindow::closeEvent(event);
}

void MainWindow::dragEnterEvent(QDragEnterEvent *event)
{
    if (event->mimeData()->hasFormat("application/x-browser-tab"))
        event->acceptProposedAction();
}

void MainWindow::dropEvent(QDropEvent *event)
{
    QByteArray encodedData = event->mimeData()->data("application/x-browser-tab");
    QUrl tabUrl = QUrl::fromEncoded(encodedData);

    // Check window identifier of tab. If matches this window's id, load the tab
    // in a new window. Otherwise, add the tab to this window
    if ((qulonglong)winId() == event->mimeData()->property("tab-origin-window-id").toULongLong())
    {
        MainWindow *win = sBrowserApplication->getNewWindow();
        win->loadUrl(tabUrl);
    }
    else
        m_tabWidget->openLinkInNewTab(tabUrl);

    event->acceptProposedAction();
}
