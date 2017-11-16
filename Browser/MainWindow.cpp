#include "BrowserApplication.h"
#include "BrowserTabWidget.h"
#include "MainWindow.h"
#include "ui_mainwindow.h"
#include "AddBookmarkDialog.h"
#include "BookmarkBar.h"
#include "BookmarkNode.h"
#include "BookmarkWidget.h"
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
#include "WebPage.h"
#include "WebView.h"

#include <functional>
#include <QActionGroup>
#include <QDir>
#include <QMessageBox>
#include <QPushButton>
#include <QRegExp>
#include <QTextEdit>
#include <QTimer>
#include <QWebElement>
#include <QWebFrame>
#include <QWebHistory>
#include <QWebInspector>
#include <QDebug>

MainWindow::MainWindow(std::shared_ptr<Settings> settings, std::shared_ptr<BookmarkManager> bookmarkManager, QWidget *parent) :
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
    m_addBookmarkDialog(nullptr)
{
    setAttribute(Qt::WA_DeleteOnClose, true);
    setToolButtonStyle(Qt::ToolButtonFollowStyle);

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

    QAction *historyItem = new QAction(title, this);
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
            agent.Value = action->data().toString();
            WebPage::setUserAgent(agent.Value);
            m_settings->setValue("CustomUserAgent", true);
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
}

void MainWindow::setupBookmarkFolder(BookmarkNode *folder, QMenu *folderMenu)
{
    if (!folderMenu)
        return;

    FaviconStorage *iconStorage = sBrowserApplication->getFaviconStorage();
    int numChildren = folder->getNumChildren();
    for (int i = 0; i < numChildren; ++i)
    {
        BookmarkNode *n = folder->getNode(i);
        if (n->getType() == BookmarkNode::Folder)
        {
            QMenu *subMenu = folderMenu->addMenu(n->getIcon(), n->getName());
            setupBookmarkFolder(n, subMenu);
        }
        else
        {
            QUrl link(n->getURL());
            QAction *item = folderMenu->addAction(iconStorage->getFavicon(QUrl(n->getURL())), n->getName());
            item->setIconVisibleInMenu(true);
            folderMenu->addAction(item);
            connect(item, &QAction::triggered, [=]() { loadUrl(link); });
        }
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
    connect(ui->actionManage_Cookies, &QAction::triggered, this, &MainWindow::openCookieManager);
    connect(ui->actionView_Downloads, &QAction::triggered, this, &MainWindow::openDownloadManager);

    // User agent sub-menu
    resetUserAgentMenu();
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

    m_tabWidget->newTab(true);
}

void MainWindow::setupToolBar()
{
    ui->toolBar->setStyleSheet("QToolBar { spacing: 3px; }");

    // Previous Page Button
    m_prevPage = new QAction(this);
    m_prevPage->setIcon(style()->standardIcon(QStyle::SP_ArrowBack, 0, this));
    m_prevPage->setToolTip(tr("Go back one page"));
    addWebProxyAction(QWebPage::Back, m_prevPage);

    // Next Page Button
    m_nextPage = new QAction(this);
    m_nextPage->setIcon(style()->standardIcon(QStyle::SP_ArrowForward, 0, this));
    m_nextPage->setToolTip(tr("Go forward one page"));
    addWebProxyAction(QWebPage::Forward, m_nextPage);

    // Stop Loading / Refresh Page dual button
    m_stopRefresh = new QAction(this);
    m_stopRefresh->setIcon(style()->standardIcon(QStyle::SP_BrowserReload));
    connect(m_stopRefresh, &QAction::triggered, [=](){
        if (WebView *view = m_tabWidget->currentWebView())
            view->reload();
    });

    // URL Bar
    m_urlInput = new URLLineEdit(ui->toolBar);
    connect(m_urlInput, &URLLineEdit::returnPressed, this, &MainWindow::goToURL);
    connect(m_urlInput, &URLLineEdit::viewSecurityInfo, this, &MainWindow::onClickSecurityInfo);

    // Quick search tool
    m_searchEngineLineEdit = new SearchEngineLineEdit(this);
    connect(m_searchEngineLineEdit, &SearchEngineLineEdit::requestPageLoad, this, &MainWindow::loadUrl);

    ui->toolBar->addAction(m_prevPage);
    ui->toolBar->addAction(m_nextPage);
    ui->toolBar->addAction(m_stopRefresh);
    ui->toolBar->addWidget(m_urlInput);
    ui->toolBar->addWidget(m_searchEngineLineEdit);
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
    m_urlInput->setURL(view->url());

    checkPageForBookmark();

    // Change current page for web proxies
    QWebPage *page = view->page();
    for (WebActionProxy *proxy : m_webActions)
        proxy->setPage(page);

    // Give focus to the url line edit widget when changing tabs
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
            m_urlInput->setText(location.toString());
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
    QTextEdit *view = new QTextEdit;
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
