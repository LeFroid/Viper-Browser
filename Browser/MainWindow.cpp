#include "BrowserApplication.h"
#include "BrowserTabWidget.h"
#include "MainWindow.h"
#include "ui_mainwindow.h"
#include "AdBlockWidget.h"
#include "BookmarkDialog.h"
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
#include <QFile>
#include <QFileDialog>
#include <QKeySequence>
#include <QLabel>
#include <QMessageBox>
#include <QMimeData>
#include <QPrinter>
#include <QPrintPreviewDialog>
#include <QPushButton>
#include <QShortcut>
#include <QSplitter>
#include <QTimer>
#include <QToolButton>

MainWindow::MainWindow(std::shared_ptr<Settings> settings, BookmarkManager *bookmarkManager, bool privateWindow, QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    m_privateWindow(privateWindow),
    m_settings(settings),
    m_bookmarkManager(bookmarkManager),
    m_bookmarkUI(nullptr),
    m_clearHistoryDialog(nullptr),
    m_tabWidget(nullptr),
    m_preferences(nullptr),
    m_bookmarkDialog(nullptr),
    m_userScriptWidget(nullptr),
    m_adBlockWidget(nullptr),
    m_faviconScript(),
    m_linkHoverLabel(nullptr)
{
    setAttribute(Qt::WA_DeleteOnClose, true);
    setToolButtonStyle(Qt::ToolButtonFollowStyle);
    setAcceptDrops(true);

    ui->setupUi(this);

    if (m_privateWindow)
        setWindowTitle("Web Browser - Private Browsing");

    setupToolBar();
    setupTabWidget();
    setupBookmarks();
    setupMenuBar();
    setupStatusBar();

    ui->dockWidget->hide();
    ui->widgetFindText->hide();
    m_tabWidget->currentWebView()->setFocus();
}

MainWindow::~MainWindow()
{
    m_urlInput->releaseParentPtr();
    delete ui;

    for (WebActionProxy *proxy : m_webActions)
        delete proxy;

    if (m_bookmarkUI)
        delete m_bookmarkUI;

    if (m_preferences)
        delete m_preferences;

    if (m_bookmarkDialog)
        delete m_bookmarkDialog;

    if (m_userScriptWidget)
        delete m_userScriptWidget;
}

bool MainWindow::isPrivate() const
{
    return m_privateWindow;
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
    connect(m_bookmarkManager, &BookmarkManager::bookmarksChanged,     this, &MainWindow::refreshBookmarkMenu);

    connect(ui->menuBookmarks, &BookmarkMenu::manageBookmarkRequest,   this, &MainWindow::openBookmarkWidget);
    connect(ui->menuBookmarks, &BookmarkMenu::loadUrlRequest,          this, &MainWindow::loadUrl);
    connect(ui->menuBookmarks, &BookmarkMenu::addPageToBookmarks,      this, &MainWindow::addPageToBookmarks);
    connect(ui->menuBookmarks, &BookmarkMenu::removePageFromBookmarks, this, &MainWindow::removePageFromBookmarks);

    // Setup bookmark bar
    ui->bookmarkBar->setBookmarkManager(m_bookmarkManager);
    connect(ui->bookmarkBar, &BookmarkBar::loadBookmark, m_tabWidget, &BrowserTabWidget::loadUrl);
    connect(ui->bookmarkBar, &BookmarkBar::loadBookmarkNewTab, m_tabWidget, &BrowserTabWidget::openLinkInNewTab);
    connect(ui->bookmarkBar, &BookmarkBar::loadBookmarkNewWindow, [=](const QUrl &url){
        m_tabWidget->openLinkInNewWindow(url, m_privateWindow);
    });
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
    addWebProxyAction(QWebEnginePage::Undo, ui->action_Undo);
    addWebProxyAction(QWebEnginePage::Redo, ui->action_Redo);
    addWebProxyAction(QWebEnginePage::Cut, ui->actionCu_t);
    addWebProxyAction(QWebEnginePage::Copy, ui->action_Copy);
    addWebProxyAction(QWebEnginePage::Paste, ui->action_Paste);

    // Add proxy for reload action in menu bar
    addWebProxyAction(QWebEnginePage::Reload, ui->actionReload);

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

    // History menu
    connect(ui->menuHistory, &HistoryMenu::loadUrl, this, &MainWindow::loadUrl);

    // History menu items
    connect(ui->menuHistory->m_actionShowHistory, &QAction::triggered, this, &MainWindow::onShowAllHistory);
    connect(ui->menuHistory->m_actionClearHistory, &QAction::triggered, [=]() {
        if (!m_clearHistoryDialog)
        {
            m_clearHistoryDialog = new ClearHistoryDialog(this);
            connect(m_clearHistoryDialog, &ClearHistoryDialog::finished, this, &MainWindow::onClearHistoryDialogFinished);
        }
        m_clearHistoryDialog->show();
    });

    // Bookmark bar setting
    ui->actionBookmark_Bar->setChecked(m_settings->getValue("EnableBookmarkBar").toBool());
    connect(ui->actionBookmark_Bar, &QAction::toggled, this, &MainWindow::toggleBookmarkBar);
    toggleBookmarkBar(ui->actionBookmark_Bar->isChecked());

    // Tools menu
    connect(ui->actionManage_Ad_Blocker, &QAction::triggered, this, &MainWindow::openAdBlockManager);
    connect(ui->actionManage_Cookies,    &QAction::triggered, this, &MainWindow::openCookieManager);
    connect(ui->actionUser_Scripts,      &QAction::triggered, this, &MainWindow::openUserScriptManager);
    connect(ui->actionView_Downloads,    &QAction::triggered, this, &MainWindow::openDownloadManager);

    // User agent sub-menu
    ui->menuUser_Agents->resetItems();

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
        QWebEnginePage *page = view->page();
        for (WebActionProxy *proxy : m_webActions)
            proxy->setPage(page);
    }
}

void MainWindow::setupTabWidget()
{
    // Create tab widget and insert into the layout
    m_tabWidget = new BrowserTabWidget(m_settings, m_privateWindow, this);
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
    // Previous Page Button
    m_prevPage = new QToolButton(ui->toolBar);
    QAction *prevPageAction = ui->toolBar->addWidget(m_prevPage);
    m_prevPage->setIcon(style()->standardIcon(QStyle::SP_ArrowBack, 0, this));
    m_prevPage->setToolTip(tr("Go back one page"));
    QMenu *buttonHistMenu = new QMenu(this);
    m_prevPage->setMenu(buttonHistMenu);
    connect(m_prevPage, &QToolButton::clicked, prevPageAction, &QAction::trigger);
    addWebProxyAction(QWebEnginePage::Back, prevPageAction);

    // Next Page Button
    m_nextPage = new QToolButton(ui->toolBar);
    QAction *nextPageAction = ui->toolBar->addWidget(m_nextPage);
    m_nextPage->setIcon(style()->standardIcon(QStyle::SP_ArrowForward, 0, this));
    m_nextPage->setToolTip(tr("Go forward one page"));
    buttonHistMenu = new QMenu(this);
    m_nextPage->setMenu(buttonHistMenu);
    connect(m_nextPage, &QToolButton::clicked, nextPageAction, &QAction::trigger);
    addWebProxyAction(QWebEnginePage::Forward, nextPageAction);

    // Stop Loading / Refresh Page dual button
    m_stopRefresh = new QAction(this);
    m_stopRefresh->setIcon(style()->standardIcon(QStyle::SP_BrowserReload));
    connect(m_stopRefresh, &QAction::triggered, [=](){
        if (WebView *view = m_tabWidget->currentWebView())
        {
            int progress = view->getProgress();
            if (progress > 0 && progress < 100)
                view->stop();
            else
                view->reload();
        }
    });

    // URL Bar
    m_urlInput = new URLLineEdit(this);
    connect(m_urlInput, &URLLineEdit::returnPressed, this, &MainWindow::goToURL);
    connect(m_urlInput, &URLLineEdit::viewSecurityInfo, this, &MainWindow::onClickSecurityInfo);
    connect(m_urlInput, &URLLineEdit::toggleBookmarkStatus, this, &MainWindow::onClickBookmarkIcon);

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

void MainWindow::setupStatusBar()
{
    m_linkHoverLabel = new QLabel(this);
    ui->statusBar->addPermanentWidget(m_linkHoverLabel, 1);
}

void MainWindow::checkPageForBookmark()
{
    WebView *view = m_tabWidget->currentWebView();
    if (!view)
        return;

    const bool isBookmarked = m_bookmarkManager->isBookmarked(view->url().toString());
    ui->menuBookmarks->setCurrentPageBookmarked(isBookmarked);
    m_urlInput->setCurrentPageBookmarked(isBookmarked);
}

void MainWindow::onTabChanged(int index)
{
    WebView *view = m_tabWidget->getWebView(index);
    if (!view)
        return;

    // Update UI elements to reflect current view
    ui->widgetFindText->setWebView(view);
    ui->widgetFindText->hide();

    // Save text in URL line edit, set widget's contents to the current URL, or if the URL is empty,
    // set to user text input
    m_urlInput->tabChanged(view);
    const QUrl &viewUrl = view->url();
    if (!viewUrl.isEmpty())
        m_urlInput->setURL(view->url());

    if (ui->dockWidget->isVisible())
        view->inspectElement();

    checkPageForBookmark();

    // Change current page for web proxies
    QWebEnginePage *page = view->page();
    for (WebActionProxy *proxy : m_webActions)
        proxy->setPage(page);

    // Give focus to the url line edit widget when changing to a blank tab
    if (m_urlInput->text().isEmpty() || m_urlInput->text().compare(QLatin1String("about:blank")) == 0)
        m_urlInput->setFocus();
    else
        view->setFocus();
}

void MainWindow::openBookmarkWidget()
{
    if (!m_bookmarkUI)
    {
        // Setup bookmark manager UI
        m_bookmarkUI = new BookmarkWidget;
        m_bookmarkUI->setBookmarkManager(m_bookmarkManager);
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
    sBrowserApplication->getCookieManager()->show();
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
    bool customTimeRange = false;
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
        case ClearHistoryDialog::CUSTOM_RANGE:
            customTimeRange = true;
        default:
            break;
    }

    if (!customTimeRange)
    {
        // Pass start time and history deletion types to BrowserApplication
        sBrowserApplication->clearHistory(m_clearHistoryDialog->getHistoryTypes(), timeRange);
    }
    else
    {
        sBrowserApplication->clearHistoryRange(m_clearHistoryDialog->getHistoryTypes(), m_clearHistoryDialog->getCustomTimeRange());
    }
}

void MainWindow::refreshBookmarkMenu()
{
    ui->menuBookmarks->resetMenu();
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

    const QString bookmarkName = view->getTitle();
    const QString bookmarkUrl = view->url().toString();
    if (m_bookmarkManager->isBookmarked(bookmarkUrl))
        return;
    m_bookmarkManager->appendBookmark(bookmarkName, bookmarkUrl);

    if (!m_bookmarkDialog)
        m_bookmarkDialog = new BookmarkDialog(m_bookmarkManager);

    m_bookmarkDialog->setDialogHeader(tr("Bookmark Added"));
    m_bookmarkDialog->setBookmarkInfo(bookmarkName, bookmarkUrl);

    // Set position of add bookmark dialog to align just under the URL bar on the right side
    m_bookmarkDialog->alignAndShow(frameGeometry(), ui->toolBar->frameGeometry(), m_urlInput->frameGeometry());
}

void MainWindow::removePageFromBookmarks(bool showDialog)
{
    WebView *view = m_tabWidget->currentWebView();
    if (!view)
        return;

    m_bookmarkManager->removeBookmark(view->url().toString());
    if (showDialog)
        QMessageBox::information(this, tr("Bookmark"), tr("Page removed from bookmarks."));
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

    const QString pageTitle = view->getTitle();
    updateTabIcon(view->icon(), m_tabWidget->indexOf(view));
    updateTabTitle(pageTitle, m_tabWidget->indexOf(view));

    if (m_tabWidget->currentWebView() == view)
    {
        m_urlInput->setURL(view->url());
        checkPageForBookmark();
        view->setFocus();
    }

    // Get favicon and inform HistoryManager of the title and favicon associated with the page's url, if not in private mode
    if (!m_privateWindow)
    {
        BrowserApplication *browserApp = sBrowserApplication;
        HistoryManager *historyMgr = browserApp->getHistoryManager();

        QIcon favicon = view->icon();
        QString pageUrl = view->url().toString();
        sBrowserApplication->getFaviconStorage()->updateIcon(view->iconUrl().toString(), pageUrl, favicon);

        historyMgr->addHistoryEntry(pageUrl);
        if (!pageTitle.isEmpty())
            historyMgr->setTitleForURL(pageUrl, pageTitle);
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
        WebView *inspectorView = dynamic_cast<WebView*>(ui->dockWidget->widget());
        if (!inspectorView)
        {
            inspectorView = new WebView(ui->dockWidget);
            ui->dockWidget->setWidget(inspectorView);
        }
        QString inspectorUrl = QString("http://127.0.0.1:%1").arg(m_settings->getValue("InspectorPort").toString());
        inspectorView->load(QUrl(inspectorUrl));
            ui->dockWidget->show();
    });
    connect(view, &WebView::linkHovered, this, &MainWindow::onLinkHovered);
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

    QString pageTitle = currentView->getTitle();
    CodeEditor *view = new CodeEditor;
    currentView->page()->toHtml([view](const QString &result){ view->setPlainText(result); });
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
    connect(&dialog, &QPrintPreviewDialog::paintRequested, [=](QPrinter *p){
        currentView->page()->print(p, [](bool){});
    });
    dialog.exec();
}

void MainWindow::onClickBookmarkIcon()
{
    WebView *currentView = m_tabWidget->currentWebView();
    if (!currentView)
        return;

    // Search for bookmark already existing, if not found, add to bookmarks, otherwise edit the existing bookmark
    BookmarkNode *node = m_bookmarkManager->getBookmark(currentView->url().toString());
    if (node == nullptr)
        addPageToBookmarks();
    else
    {
        if (!m_bookmarkDialog)
            m_bookmarkDialog = new BookmarkDialog(m_bookmarkManager);

        m_bookmarkDialog->setDialogHeader(tr("Bookmark"));
        m_bookmarkDialog->setBookmarkInfo(node->getName(), node->getURL(), node->getParent());

        // Set position of add bookmark dialog to align just under the URL bar on the right side
        m_bookmarkDialog->alignAndShow(frameGeometry(), ui->toolBar->frameGeometry(), m_urlInput->frameGeometry());
    }
    //m_urlInput->setCurrentPageBookmarked(!isBookmarked);
}

WebView *MainWindow::getNewTabWebView()
{
    return m_tabWidget->newTab();
}

BrowserTabWidget *MainWindow::getTabWidget() const
{
    return m_tabWidget;
}

int MainWindow::getToolbarHeight() const
{
    return ui->toolBar->height();
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
    {
        event->acceptProposedAction();
        return;
    }

    QMainWindow::dragEnterEvent(event);
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

void MainWindow::onLinkHovered(const QUrl &url)
{
    // Hide if url is empty
    if (url.isEmpty())
        m_linkHoverLabel->setText(QString());
    else
        m_linkHoverLabel->setText(url.toString());
}
