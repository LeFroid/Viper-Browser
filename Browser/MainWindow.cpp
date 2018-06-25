#include "BrowserApplication.h"
#include "BrowserTabWidget.h"
#include "MainWindow.h"
#include "ui_MainWindow.h"
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
#include <QMouseEvent>
#include <QPrinter>
#include <QPrintPreviewDialog>
#include <QPushButton>
#include <QShortcut>
#include <QtGlobal>
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

    // load favicon script
    QFile f(":/GetFavicon.js");
    if (f.open(QIODevice::ReadOnly))
    {
        m_faviconScript = f.readAll();
        f.close();
    }

    ui->setupUi(this);
    ui->toolBar->setMinHeights(ui->toolBar->height() + 3);

    if (m_privateWindow)
        setWindowTitle("Web Browser - Private Browsing");

    setupStatusBar();
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

WebView *MainWindow::currentWebView() const
{
    return m_tabWidget->currentWebView();
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
    connect(ui->action_Quit,        &QAction::triggered, sBrowserApplication, &BrowserApplication::quit);
    connect(ui->actionOpen_File,    &QAction::triggered, this, &MainWindow::openFileInBrowser);
    connect(ui->action_Print,       &QAction::triggered, this, &MainWindow::printTabContents);

    addWebProxyAction(WebPage::SavePage, ui->action_Save_Page_As);

    // Find action
    connect(ui->action_Find, &QAction::triggered, this, &MainWindow::onFindTextAction);

    // Add proxy functionality to edit menu actions
    addWebProxyAction(WebPage::Undo, ui->action_Undo);
    addWebProxyAction(WebPage::Redo, ui->action_Redo);
    addWebProxyAction(WebPage::Cut, ui->actionCu_t);
    addWebProxyAction(WebPage::Copy, ui->action_Copy);
    addWebProxyAction(WebPage::Paste, ui->action_Paste);

    // Add proxy for reload action in menu bar
    addWebProxyAction(QWebEnginePage::Reload, ui->actionReload);

    // Zoom in / out / reset slots
    connect(ui->actionZoom_In, &QAction::triggered, m_tabWidget, &BrowserTabWidget::zoomInCurrentView);
    connect(ui->actionZoom_Out, &QAction::triggered, m_tabWidget, &BrowserTabWidget::zoomOutCurrentView);
    connect(ui->actionReset_Zoom, &QAction::triggered, m_tabWidget, &BrowserTabWidget::resetZoomCurrentView);

    // Full screen view mode
    connect(ui->action_Full_Screen, &QAction::triggered, this, &MainWindow::onToggleFullScreen);

    // Preferences window
    connect(ui->actionPreferences, &QAction::triggered, this, &MainWindow::openPreferences);

    // View-related
    connect(ui->actionPage_So_urce, &QAction::triggered, this, &MainWindow::onRequestViewSource);

    // History menu
    connect(ui->menuHistory, &HistoryMenu::loadUrl, this, &MainWindow::loadUrl);

    // History menu items
    connect(ui->menuHistory->m_actionShowHistory, &QAction::triggered, this, &MainWindow::onShowAllHistory);
    connect(ui->menuHistory->m_actionClearHistory, &QAction::triggered, this, &MainWindow::openClearHistoryDialog);

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
    connect(m_tabWidget, &BrowserTabWidget::loadProgress, [=](int progress) {
        if (progress > 0 && progress < 100)
            m_linkHoverLabel->setText(QString("%1% loaded...").arg(progress));
        else
            m_linkHoverLabel->setText(QString());
    });
    ui->toolBar->bindWithTabWidget();

    // Add first tab
    m_tabWidget->newTab(true);
}

void MainWindow::setupStatusBar()
{
    m_linkHoverLabel = new QLabel(this);
    ui->statusBar->addPermanentWidget(m_linkHoverLabel, 1);
    //ui->statusBar->hide();
}

void MainWindow::checkPageForBookmark()
{
    WebView *view = m_tabWidget->currentWebView();
    if (!view)
        return;

    const bool isBookmarked = m_bookmarkManager->isBookmarked(view->url().toString());
    ui->menuBookmarks->setCurrentPageBookmarked(isBookmarked);
    ui->toolBar->getURLWidget()->setCurrentPageBookmarked(isBookmarked);
}

void MainWindow::onTabChanged(int index)
{
    WebView *view = m_tabWidget->getWebView(index);
    if (!view)
        return;

    // Update UI elements to reflect current view
    ui->widgetFindText->setWebView(view);
    ui->widgetFindText->hide();

    // Save text in URL line edit, set widget's contents to the current URL, or if the URL is empty or "about:blank",
    // set to user text input
    URLLineEdit *urlInput = ui->toolBar->getURLWidget();
    urlInput->tabChanged(view);

    const bool isBlankPage = view->isOnBlankPage();
    if (urlInput->text().isEmpty() || !isBlankPage)
        urlInput->setURL(view->url());        

    // Show dock widget with dev tools UI if it was opened in the last tab
    if (ui->dockWidget->isVisible())
        view->inspectElement();

    checkPageForBookmark();

    // Change current page for web proxies
    QWebEnginePage *page = view->page();
    for (WebActionProxy *proxy : m_webActions)
        proxy->setPage(page);

    // Give focus to the url line edit widget when changing to a blank tab
    if (urlInput->text().isEmpty() || isBlankPage)
    {
        urlInput->setFocus();

        if (urlInput->text().compare(QLatin1String("viper://blank")) == 0)
            urlInput->selectAll();
    }
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
    m_bookmarkDialog->alignAndShow(frameGeometry(), ui->toolBar->frameGeometry(), ui->toolBar->getURLWidget()->frameGeometry());
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

void MainWindow::openClearHistoryDialog()
{
    if (!m_clearHistoryDialog)
    {
        m_clearHistoryDialog = new ClearHistoryDialog(this);
        connect(m_clearHistoryDialog, &ClearHistoryDialog::finished, this, &MainWindow::onClearHistoryDialogFinished);
    }

    m_clearHistoryDialog->show();
}

void MainWindow::openPreferences()
{
    if (!m_preferences)
    {
        m_preferences = new Preferences(m_settings);

        connect(m_preferences, &Preferences::clearHistoryRequested, this, &MainWindow::openClearHistoryDialog);
        connect(m_preferences, &Preferences::viewHistoryRequested, this, &MainWindow::onShowAllHistory);
    }

    m_preferences->loadSettings();
    m_preferences->show();
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

void MainWindow::onLoadFinished(bool /*ok*/)
{
    WebView *view = qobject_cast<WebView*>(sender());
    if (!view)
        return;

    const QString pageTitle = view->getTitle();
    QIcon icon = view->icon();
    if (icon.isNull())
        icon = sBrowserApplication->getFaviconStorage()->getFavicon(view->url());
    updateTabIcon(icon, m_tabWidget->indexOf(view));
    updateTabTitle(pageTitle, m_tabWidget->indexOf(view));

    if (m_tabWidget->currentWebView() == view)
    {
        ui->toolBar->getURLWidget()->setURL(view->url());
        checkPageForBookmark();
        view->setFocus();
    }

    if (m_privateWindow)
        return;

    // Get favicon and inform HistoryManager of the title and favicon associated with the page's url
    BrowserApplication *browserApp = sBrowserApplication;
    HistoryManager *historyMgr = browserApp->getHistoryManager();

    QIcon favicon = view->icon();
    QString pageUrl = view->url().toString();
    QString pageUrlNoPath = view->url().toString(QUrl::RemovePath);
    QString pageScheme = view->url().scheme();
    const bool isBlankPage = view->isOnBlankPage();

    if (!isBlankPage && !pageUrl.isEmpty())
        historyMgr->addHistoryEntry(pageUrl, pageTitle);

    // Attempt to fetch the URL of the favicon from the page
    view->page()->runJavaScript(m_faviconScript, [=](const QVariant &v) {
        if (v.isNull() || !v.canConvert<QString>())
            return;

        QString iconRef = v.toString();
        if (iconRef.startsWith(QLatin1String("//")))
        {
            iconRef.prepend(pageScheme + QChar(':'));
        }
        else if (iconRef.startsWith('/'))
            iconRef.prepend(pageUrlNoPath);
        sBrowserApplication->getFaviconStorage()->updateIcon(iconRef, pageUrl, favicon);
    });
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
    connect(view, &WebView::loadFinished, this, &MainWindow::onLoadFinished);
    connect(view, &WebView::inspectElement, [=]() {
        WebView *inspectorView = dynamic_cast<WebView*>(ui->dockWidget->widget());
        if (!inspectorView)
        {
            inspectorView = new WebView(ui->dockWidget);
            inspectorView->setObjectName(QLatin1String("inspectorView"));
            inspectorView->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
            ui->dockWidget->setWidget(inspectorView);
#if (QTWEBENGINECORE_VERSION >= QT_VERSION_CHECK(5, 11, 0))
            inspectorView->setContextMenuPolicy(Qt::CustomContextMenu);
            connect(inspectorView, &WebView::openRequest, this, &MainWindow::openLinkNewTab);
            connect(inspectorView, &WebView::openInNewTabRequest, this, &MainWindow::openLinkNewTab);
#endif
        }
#if (QTWEBENGINECORE_VERSION >= QT_VERSION_CHECK(5, 11, 0))
        WebPage *inspectorPage = dynamic_cast<WebPage*>(inspectorView->page());
        inspectorPage->setInspectedPage(view->page());
#else
        QString inspectorUrl = QString("http://127.0.0.1:%1").arg(m_settings->getValue("InspectorPort").toString());
        inspectorView->load(QUrl(inspectorUrl));
#endif
        ui->dockWidget->show();
    });
    connect(view, &WebView::linkHovered, this, &MainWindow::onLinkHovered);

    if (WebPage *page = dynamic_cast<WebPage*>(view->page()))
    {
        connect(page, &WebPage::printPageRequest, this, &MainWindow::printTabContents);
    }
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
    {
        showFullScreen();
        ui->menuBar->hide();
        ui->toolBar->hide();
        ui->statusBar->hide();
        m_tabWidget->tabBar()->hide();
    }
    else
    {
        showMaximized();

        ui->menuBar->show();
        ui->toolBar->show();
        m_tabWidget->tabBar()->show();
        ui->statusBar->show();
    }
}

void MainWindow::onMouseMoveFullscreen(int y)
{
    const bool isToolBarHidden = ui->toolBar->isHidden();
    if (y <= 5 && isToolBarHidden)
    {
        ui->menuBar->show();
        ui->toolBar->show();
        m_tabWidget->tabBar()->show();
    }
    else if (!isToolBarHidden)
    {
        if (y > ui->toolBar->pos().y() + ui->toolBar->height() + m_tabWidget->tabBar()->height() + 10)
        {
            ui->menuBar->hide();
            ui->toolBar->hide();
            m_tabWidget->tabBar()->hide();
        }
    }
}

void MainWindow::printTabContents()
{
    WebPage *page = qobject_cast<WebPage*>(sender());
    if (!page)
    {
        if (WebView *currentView = m_tabWidget->currentWebView())
            page = qobject_cast<WebPage*>(currentView->page());
    }
    if (!page)
        return;

    QPrinter printer(QPrinter::HighResolution);
    printer.setPaperSize(QPrinter::Letter);
    printer.setFullPage(true);
    QPrintPreviewDialog dialog(&printer, this);
    dialog.setWindowTitle(tr("Print Document"));
    connect(&dialog, &QPrintPreviewDialog::paintRequested, [=](QPrinter *p){
        page->print(p, [](bool){});
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
        m_bookmarkDialog->alignAndShow(frameGeometry(), ui->toolBar->frameGeometry(), ui->toolBar->getURLWidget()->frameGeometry());
    }
}

WebView *MainWindow::getNewTabWebView(bool makeCurrent)
{
    return m_tabWidget->newTab(makeCurrent);
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
    m_linkHoverLabel->setText(url.toString());
}
