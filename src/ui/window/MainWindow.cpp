#include "BrowserApplication.h"
#include "BrowserTabWidget.h"
#include "MainWindow.h"
#include "ui_MainWindow.h"
#include "AdBlockLogDisplay.h"
#include "AdBlockManager.h"
#include "AutoFillCredentialsView.h"
#include "BookmarkDialog.h"
#include "BookmarkBar.h"
#include "BookmarkNode.h"
#include "BookmarkManager.h"
#include "BookmarkWidget.h"
#include "ClearHistoryDialog.h"
#include "CookieJar.h"
#include "HistoryWidget.h"
#include "HttpRequest.h"
#include "Preferences.h"
#include "SecurityManager.h"
#include "SearchEngineLineEdit.h"
#include "ToolMenu.h"
#include "URLLineEdit.h"
#include "ViewSourceWindow.h"
#include "WebInspector.h"
#include "WebPage.h"
#include "WebPageTextFinder.h"
#include "WebView.h"
#include "WebWidget.h"

#include <functional>
#include <QActionGroup>
#include <QCloseEvent>
#include <QDir>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QFile>
#include <QFileDialog>
#include <QFuture>
#include <QFutureWatcher>
#include <QKeySequence>
#include <QLabel>
#include <QMessageBox>
#include <QMimeData>
#include <QMouseEvent>
#include <QPrintPreviewDialog>
#include <QPushButton>
#include <QScreen>
#include <QShortcut>
#include <QTabBar>
#include <QTimer>
#include <QtGlobal>
#include <QToolButton>
#include <QtConcurrent>

MainWindow::MainWindow(const ViperServiceLocator &serviceLocator, bool privateWindow, QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    m_privateWindow(privateWindow),
    m_settings(serviceLocator.getServiceAs<Settings>("Settings")),
    m_serviceLocator(serviceLocator),
    m_bookmarkManager(serviceLocator.getServiceAs<BookmarkManager>("BookmarkManager")),
    m_clearHistoryDialog(nullptr),
    m_tabWidget(nullptr),
    m_bookmarkDialog(nullptr),
    m_linkHoverLabel(nullptr),
    m_closing(false),
    m_printer(QPrinter::ScreenResolution),
    m_printLoop(),
    m_isInPrintPreview(false)
{
    setAttribute(Qt::WA_DeleteOnClose, true);
    setToolButtonStyle(Qt::ToolButtonFollowStyle);
    setAcceptDrops(true);

    ui->setupUi(this);
    ui->toolBar->setMinHeights(ui->toolBar->height() + 3);
    ui->toolBar->setServiceLocator(serviceLocator);

    if (m_privateWindow)
        setWindowTitle("Web Browser - Private Browsing");

    QScreen *mainScreen = sBrowserApplication->primaryScreen();
    const int availableWidth = mainScreen->availableGeometry().width(),
              availableHeight = mainScreen->availableGeometry().height();
    setGeometry(availableWidth / 16, availableHeight / 16, availableWidth * 7 / 8, availableHeight * 7 / 8);

    ui->widgetFindText->setTextFinder(std::make_unique<WebPageTextFinder>());

    setupStatusBar();
    setupTabWidget();
    setupBookmarks();
    setupMenuBar();

    connect(ui->toolBar, &NavigationToolBar::clickedAdBlockButton, this, &MainWindow::openAdBlockLogDisplay);
    connect(ui->toolBar, &NavigationToolBar::requestManageAdBlockSubscriptions, ui->menuTools, &ToolMenu::openAdBlockManager);

    connect(ui->toolBar->getURLWidget(), &URLLineEdit::loadRequested, m_tabWidget, &BrowserTabWidget::loadUrlFromUrlBar);

    connect(ui->dockWidget, &QDockWidget::visibilityChanged, this, [this](bool visible) {
        if (WebWidget *webWidget = m_tabWidget->currentWebWidget())
        {
            // First check if the inspector has been instantiated
            // (calling getInspector() will instantiate the inspector)
            if (visible || webWidget->isInspectorActive())
                webWidget->getInspector()->setActive(visible);
        }
    });

    ui->dockWidget->hide();
    ui->widgetFindText->hide();
    m_tabWidget->currentWebWidget()->setFocus();
}

MainWindow::~MainWindow()
{
    delete ui;

    for (WebActionProxy *proxy : qAsConst(m_webActions))
        delete proxy;

    if (m_bookmarkDialog)
        delete m_bookmarkDialog;
}

bool MainWindow::isPrivate() const
{
    return m_privateWindow;
}

WebWidget *MainWindow::currentWebWidget() const
{
    return m_tabWidget->currentWebWidget();
}

void MainWindow::prepareToClose()
{
    m_closing.store(true);
}

void MainWindow::loadBlankPage()
{
    m_tabWidget->currentWebWidget()->loadBlankPage();
}

void MainWindow::loadUrl(const QUrl &url)
{
    m_tabWidget->loadUrl(url);
}

void MainWindow::loadHttpRequest(const HttpRequest &request)
{
    m_tabWidget->currentWebWidget()->load(request);
}

void MainWindow::openLinkNewTab(const QUrl &url)
{
    m_tabWidget->openLinkInNewBackgroundTab(url);
}

void MainWindow::openLinkNewWindow(const QUrl &url)
{
    m_tabWidget->openLinkInNewWindow(url, m_privateWindow);
}

void MainWindow::setupBookmarks()
{
    connect(m_bookmarkManager, &BookmarkManager::bookmarksChanged, this, &MainWindow::checkPageForBookmark);

    ui->menuBookmarks->setBookmarkManager(m_bookmarkManager);
    connect(ui->menuBookmarks, &BookmarkMenu::manageBookmarkRequest,   this, &MainWindow::openBookmarkWidget);
    connect(ui->menuBookmarks, &BookmarkMenu::loadUrlRequest,          this, &MainWindow::loadUrl);
    connect(ui->menuBookmarks, &BookmarkMenu::addPageToBookmarks,      this, &MainWindow::addPageToBookmarks);
    connect(ui->menuBookmarks, &BookmarkMenu::removePageFromBookmarks, this, &MainWindow::removePageFromBookmarks);

    // Setup bookmark bar
    ui->bookmarkBar->setBookmarkManager(m_bookmarkManager);
    connect(ui->bookmarkBar, &BookmarkBar::loadBookmark, m_tabWidget, &BrowserTabWidget::loadUrl);
    connect(ui->bookmarkBar, &BookmarkBar::loadBookmarkNewTab, m_tabWidget, &BrowserTabWidget::openLinkInNewBackgroundTab);
    connect(ui->bookmarkBar, &BookmarkBar::loadBookmarkNewWindow, [=](const QUrl &url){
        m_tabWidget->openLinkInNewWindow(url, m_privateWindow);
    });
}

void MainWindow::setupMenuBar()
{
    // File menu slots
    connect(ui->actionNew_Tab, &QAction::triggered, [=](bool){
        m_tabWidget->newBackgroundTab();
    });
    connect(ui->actionNew_Window,         &QAction::triggered, sBrowserApplication, &BrowserApplication::getNewWindow);
    connect(ui->actionNew_Private_Window, &QAction::triggered, sBrowserApplication, &BrowserApplication::getNewPrivateWindow);
    connect(ui->actionClose_Tab,          &QAction::triggered, m_tabWidget,         &BrowserTabWidget::closeCurrentTab);
    connect(ui->action_Quit,              &QAction::triggered, sBrowserApplication, &BrowserApplication::prepareToQuit);
    //connect(ui->action_Save_Page_As, &QAction::triggered, this, &MainWindow::onSavePageTriggered);
    addWebProxyAction(WebPage::SavePage, ui->action_Save_Page_As);

    // Add proxy functionality to edit menu actions
    addWebProxyAction(WebPage::Undo,  ui->action_Undo);
    addWebProxyAction(WebPage::Redo,  ui->action_Redo);
    addWebProxyAction(WebPage::Cut,   ui->actionCu_t);
    addWebProxyAction(WebPage::Copy,  ui->action_Copy);
    addWebProxyAction(WebPage::Paste, ui->action_Paste);

    // Add proxy for reload action in menu bar
    addWebProxyAction(WebPage::Reload, ui->actionReload);

    // Zoom in / out / reset slots
    connect(ui->actionZoom_In,    &QAction::triggered, m_tabWidget, &BrowserTabWidget::zoomInCurrentView);
    connect(ui->actionZoom_Out,   &QAction::triggered, m_tabWidget, &BrowserTabWidget::zoomOutCurrentView);
    connect(ui->actionReset_Zoom, &QAction::triggered, m_tabWidget, &BrowserTabWidget::resetZoomCurrentView);

    // History menu
    ui->menuHistory->setServiceLocator(m_serviceLocator);
    connect(ui->menuHistory, &HistoryMenu::loadUrl, this, &MainWindow::loadUrl);

    // History menu items
    connect(ui->menuHistory->m_actionShowHistory,  &QAction::triggered, this, &MainWindow::onShowAllHistory);
    connect(ui->menuHistory->m_actionClearHistory, &QAction::triggered, this, &MainWindow::openClearHistoryDialog);

    // Bookmark bar setting
    ui->actionBookmark_Bar->setChecked(m_settings->getValue(BrowserSetting::EnableBookmarkBar).toBool());
    connect(ui->actionBookmark_Bar, &QAction::toggled, this, &MainWindow::toggleBookmarkBar);
    toggleBookmarkBar(ui->actionBookmark_Bar->isChecked());

    // Tools menu
    ui->menuTools->setServiceLocator(m_serviceLocator);

    // Help menu
    connect(ui->actionAbout, &QAction::triggered, [=](){
        QString appName = sBrowserApplication->applicationName();
        QString appVersion = sBrowserApplication->applicationVersion();
        QMessageBox::about(this, tr("About %1").arg(appName), tr("%1 - Version %2\nDeveloped by Timothy Vaccarelli").arg(appName, appVersion));
    });
    connect(ui->actionAbout_Qt, &QAction::triggered, [=](){
        QMessageBox::aboutQt(this, tr("About Qt"));
    });

    // Set web page for proxy actions (called automatically during onTabChanged event after all UI elements are set up)
    if (WebWidget *view = m_tabWidget->getWebWidget(0))
    {
        WebPage *page = view->page();
        for (WebActionProxy *proxy : qAsConst(m_webActions))
            proxy->setPage(page);
    }
}

void MainWindow::setupTabWidget()
{
    // Create tab widget and insert into the layout
    m_tabWidget = new BrowserTabWidget(m_serviceLocator, m_privateWindow, this);
    ui->verticalLayout->insertWidget(ui->verticalLayout->indexOf(ui->widgetFindText), m_tabWidget);

    // Add change tab slot after removing dummy tabs to avoid segfaulting
    connect(m_tabWidget, &BrowserTabWidget::viewChanged, this, &MainWindow::onTabChanged);

    // Some singals emitted by WebViews (which are managed largely by the tab widget) must be dealt with in the MainWindow
    connect(m_tabWidget, &BrowserTabWidget::newTabCreated, this, &MainWindow::onNewTabCreated);
    connect(m_tabWidget, &BrowserTabWidget::loadProgress, [=](int progress) {
        if (progress > 0 && progress < 100)
        {
            m_linkHoverLabel->setText(tr("%1% loaded...").arg(progress));
            //ui->statusBar->show();
        }
        else
        {
            m_linkHoverLabel->setText(QString());
            //ui->statusBar->hide();
        }
    });
    if (!m_privateWindow)
    {
        connect(m_tabWidget, &BrowserTabWidget::titleChanged, [this](const QString &title){
            setWindowTitle(tr("%1 - Web Browser").arg(title));
        });
    }
    ui->toolBar->bindWithTabWidget();

    // Add first tab
    static_cast<void>(m_tabWidget->newTab());
}

void MainWindow::setupStatusBar()
{
    m_linkHoverLabel = new QLabel(this);
    ui->statusBar->addPermanentWidget(m_linkHoverLabel, 1);
    //ui->statusBar->hide();
}

void MainWindow::checkPageForBookmark()
{
    WebWidget *ww = m_tabWidget->currentWebWidget();
    if (!ww)
        return;

    const QUrl pageUrl = ww->url();
    QFutureWatcher<bool> *watcher = new QFutureWatcher<bool>(this);
    connect(watcher, &QFutureWatcher<bool>::finished, this, [this, pageUrl, watcher](){
        const bool isBookmarked = watcher->future().result();
        BookmarkNode *n = isBookmarked ? m_bookmarkManager->getBookmark(pageUrl) : nullptr;
        ui->menuBookmarks->setCurrentPageBookmarked(isBookmarked);
        ui->toolBar->getURLWidget()->setCurrentPageBookmarked(isBookmarked, n);
        watcher->deleteLater();
    });
    QFuture<bool> b = QtConcurrent::run(&BookmarkManager::isBookmarked, m_bookmarkManager, pageUrl);
    watcher->setFuture(b);
}

void MainWindow::onTabChanged(int index)
{
    WebWidget *ww = m_tabWidget->getWebWidget(index);
    if (!ww)
        return;

    // Update text finder
    ui->widgetFindText->clearLabels();
    ui->widgetFindText->hide();

    if (WebPageTextFinder *textFinder = dynamic_cast<WebPageTextFinder*>(ui->widgetFindText->getTextFinder()))
        textFinder->setWebPage(ww->page());

    // Update URL bar
    URLLineEdit *urlInput = ui->toolBar->getURLWidget();
    urlInput->tabChanged(ww);

    // Handle web inspector state
    if (!ww->isHibernating() && ww->isInspectorActive())
    {
        ui->dockWidget->setWidget(ww->getInspector());
        ui->dockWidget->show();
    }
    else
        ui->dockWidget->hide();

    checkPageForBookmark();

    // Redirect web proxies to current page
    WebPage *page = ww->page();
    for (WebActionProxy *proxy : qAsConst(m_webActions))
        proxy->setPage(page);

    // Give focus to the url line edit widget when changing to a blank tab
    if (urlInput->text().isEmpty() || ww->isOnBlankPage())
    {
        urlInput->setFocus();

        if (urlInput->text().startsWith(QLatin1String("viper:")))
            urlInput->selectAll();
    }
    else
        ww->setFocus();

    // Add current page title to the window title if not in private mode
    if (!m_privateWindow)
        setWindowTitle(tr("%1 - Web Browser").arg(ww->getTitle()));
}

void MainWindow::openBookmarkWidget()
{
    BookmarkWidget *bookmarkWidget = new BookmarkWidget;
    bookmarkWidget->setBookmarkManager(m_bookmarkManager);
    connect(bookmarkWidget, &BookmarkWidget::openBookmark, m_tabWidget, &BrowserTabWidget::loadUrl);
    connect(bookmarkWidget, &BookmarkWidget::openBookmarkNewTab, m_tabWidget, &BrowserTabWidget::openLinkInNewBackgroundTab);
    connect(bookmarkWidget, &BookmarkWidget::openBookmarkNewWindow, this, &MainWindow::openLinkNewWindow);

    bookmarkWidget->show();
    bookmarkWidget->raise();
    bookmarkWidget->activateWindow();

    // Force a resize event on the widget, so the bookmark table renders the columns proportionally
    bookmarkWidget->resize(bookmarkWidget->width() + 2, bookmarkWidget->height());
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
            break;
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

void MainWindow::addPageToBookmarks()
{
    WebWidget *ww = m_tabWidget->currentWebWidget();
    if (!ww)
        return;

    const QString bookmarkName = ww->getTitle();
    const QUrl bookmarkUrl = ww->url();

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
    WebWidget *ww = m_tabWidget->currentWebWidget();
    if (!ww)
        return;

    m_bookmarkManager->removeBookmark(ww->url());
    if (showDialog)
        QMessageBox::information(this, tr("Bookmark"), tr("Page removed from bookmarks."));
}

void MainWindow::toggleBookmarkBar(bool enabled)
{
    if (enabled)
        ui->bookmarkBar->show();
    else
        ui->bookmarkBar->hide();

    m_settings->setValue(BrowserSetting::EnableBookmarkBar, enabled);
}

void MainWindow::onFindTextAction()
{
    ui->widgetFindText->show();
    auto lineEdit = ui->widgetFindText->getLineEdit();
    lineEdit->setFocus();
    lineEdit->selectAll();
}

void MainWindow::openAdBlockLogDisplay()
{
    AdBlockLogDisplay *logDisplay = new AdBlockLogDisplay(m_serviceLocator.getServiceAs<adblock::AdBlockManager>("AdBlockManager"));
    logDisplay->setLogTableFor(m_tabWidget->currentWebWidget()->url().adjusted(QUrl::RemoveFragment));
    logDisplay->show();
    logDisplay->raise();
    logDisplay->activateWindow();
}

void MainWindow::openAutoFillCredentialsView()
{
    AutoFillCredentialsView *credentialsView = new AutoFillCredentialsView;
    credentialsView->show();
    credentialsView->raise();
    credentialsView->activateWindow();
}

void MainWindow::openAutoFillExceptionsView()
{
    //TODO: UI for this
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
    CookieJar *cookieJar = m_serviceLocator.getServiceAs<CookieJar>("CookieJar");
    Preferences *preferences = new Preferences(m_settings, cookieJar);

    connect(preferences, &Preferences::clearHistoryRequested, this, &MainWindow::openClearHistoryDialog);
    connect(preferences, &Preferences::viewHistoryRequested,  this, &MainWindow::onShowAllHistory);
    connect(preferences, &Preferences::viewSavedCredentialsRequested, this, &MainWindow::openAutoFillCredentialsView);
    connect(preferences, &Preferences::viewAutoFillExceptionsRequested, this, &MainWindow::openAutoFillExceptionsView);

    preferences->show();
    preferences->raise();
    preferences->activateWindow();
}

void MainWindow::openFileInBrowser()
{
    QString fileName = QFileDialog::getOpenFileName(this, tr("Open File"), QDir::homePath());
    if (!fileName.isEmpty())
        loadUrl(QUrl(QString("file://%1").arg(fileName)));
}

void MainWindow::onLoadFinished(bool ok)
{
    WebWidget *ww = qobject_cast<WebWidget*>(sender());
    if (!ww)
        return;

    if (m_tabWidget->currentWebWidget() != ww)
        return;

    auto urlWidget = ui->toolBar->getURLWidget();
    if (!urlWidget->isModified() && urlWidget->cursorPosition() == 0)
        urlWidget->setURL(ww->url());

    checkPageForBookmark();

    if (ui->widgetFindText->isVisible()
            && !ui->widgetFindText->getLineEdit()->text().isEmpty())
    {
        if (WebView *view = ww->view())
            view->findText(QString());
    }

    if (!ww->isOnBlankPage()
            && !ui->widgetFindText->getLineEdit()->hasFocus()
            && !(urlWidget->hasFocus() || urlWidget->isModified()))
        ww->setFocus();

    // Add current page title to the window title if not in private mode
    if (ok && !m_privateWindow)
        setWindowTitle(tr("%1 - Web Browser").arg(ww->getTitle()));
}

void MainWindow::onShowAllHistory()
{
    HistoryWidget *histWidget = new HistoryWidget;
    histWidget->setServiceLocator(m_serviceLocator);
    histWidget->loadHistory();

    connect(histWidget, &HistoryWidget::openLink, m_tabWidget, &BrowserTabWidget::loadUrl);
    connect(histWidget, &HistoryWidget::openLinkNewTab, m_tabWidget, &BrowserTabWidget::openLinkInNewBackgroundTab);
    connect(histWidget, &HistoryWidget::openLinkNewWindow, this, &MainWindow::openLinkNewWindow);

    histWidget->show();
}

void MainWindow::onNewTabCreated(WebWidget *ww)
{
    // Connect signals to slots for UI updates (page title, icon changes)
    connect(ww, &WebWidget::aboutToWake, [this,ww](){
        if (m_tabWidget->currentWebWidget() == ww)
        {
            ui->widgetFindText->clearLabels();
            if (WebPageTextFinder *textFinder = dynamic_cast<WebPageTextFinder*>(ui->widgetFindText->getTextFinder()))
                textFinder->setWebPage(ww->page());
        }
    });
    connect(ww, &WebWidget::loadFinished,   this, &MainWindow::onLoadFinished);
    connect(ww, &WebWidget::inspectElement, this, &MainWindow::openInspector);
    connect(ww, &WebWidget::linkHovered,    this, &MainWindow::onLinkHovered);

    if (WebView *view = ww->view())
    {
        connect(view, &WebView::printRequested, this, &MainWindow::printTabContents);
        connect(view, &WebView::printFinished,  this, &MainWindow::onPrintFinished);
    }
}

void MainWindow::openInspector()
{
    WebWidget *webWidget = qobject_cast<WebWidget*>(sender());
    if (!webWidget)
        return;

    WebInspector *inspector = webWidget->getInspector();
    if (qobject_cast<WebInspector*>(ui->dockWidget->widget()) != inspector)
        ui->dockWidget->setWidget(inspector);

    if (!ui->dockWidget->isVisible())
        ui->dockWidget->show();

    webWidget->page()->triggerAction(WebPage::InspectElement);

#if (QTWEBENGINECORE_VERSION >= QT_VERSION_CHECK(5, 11, 0))
    Qt::ConnectionType uniqueConnection = Qt::ConnectionType(Qt::AutoConnection | Qt::UniqueConnection);
    connect(inspector, &WebInspector::openRequest,  this, &MainWindow::openLinkNewTab, uniqueConnection);
    connect(inspector, &WebInspector::openInNewTab, this, &MainWindow::openLinkNewTab, uniqueConnection);
#endif
}

void MainWindow::onClickSecurityInfo()
{
    WebWidget *currentView = m_tabWidget->currentWebWidget();
    if (!currentView)
        return;
    SecurityManager::instance().showSecurityInfo(currentView->url());
}

void MainWindow::onRequestViewSource()
{
    WebWidget *currentView = m_tabWidget->currentWebWidget();
    if (!currentView)
        return;

    ViewSourceWindow *sourceWindow = new ViewSourceWindow(currentView->getTitle());
    sourceWindow->setWebPage(currentView->page());
    sourceWindow->show();
    sourceWindow->raise();
    sourceWindow->activateWindow();
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

//TODO: move printTabContents & onPrintPreviewRequested functionality into a separate PrintHandler class
void MainWindow::printTabContents()
{
    WebView *view = qobject_cast<WebView*>(sender());
    if (!view)
    {
        if (WebWidget *ww = m_tabWidget->currentWebWidget())
            view = ww->view();
    }
    if (!view)
        return;

    m_printer.setFullPage(true);
    QPrintPreviewDialog dialog(&m_printer, this);
    dialog.setWindowTitle(tr("Print Document"));
    connect(&dialog, &QPrintPreviewDialog::paintRequested, this, [this,view](QPrinter *p) {
        onPrintPreviewRequested(p, view);
    });
    dialog.exec();
}

void MainWindow::onPrintPreviewRequested(QPrinter *printer, WebView *view)
{
    if (!view || m_isInPrintPreview)
        return;

    m_isInPrintPreview = true;
    view->print(printer);
    m_printLoop.exec();
    m_isInPrintPreview = false;
}

void MainWindow::onPrintFinished(bool /*success*/)
{
    m_printLoop.quit();
}

void MainWindow::onClickBookmarkIcon()
{
    WebWidget *currentView = m_tabWidget->currentWebWidget();
    if (!currentView)
        return;

    // Search for bookmark already existing, if not found, add to bookmarks, otherwise edit the existing bookmark
    BookmarkNode *node = ui->toolBar->getURLWidget()->getBookmarkNode();
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

BrowserTabWidget *MainWindow::getTabWidget() const
{
    return m_tabWidget;
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if (m_privateWindow || m_closing)
    {
        QMainWindow::closeEvent(event);
        return;
    }

    int numTabsOpen = m_tabWidget->count();
    if (numTabsOpen > 1)
    {
        //TODO: Consider asking the user if they want this prompt or not; save their preference.
        auto response = QMessageBox::question(this,tr("Warning"),
                                              tr("You are about to close %1 tabs. Do you wish to proceed?").arg(numTabsOpen));
        if (response == QMessageBox::No)
        {
            event->ignore();
            return;
        }
    }

    StartupMode mode = static_cast<StartupMode>(m_settings->getValue(BrowserSetting::StartupMode).toInt());
    if (mode == StartupMode::RestoreSession)
    {
        Q_EMIT aboutToClose();
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
    WebState webState;
    webState.deserialize(encodedData);

    const qulonglong otherWinId = event->mimeData()->property("tab-origin-window-id").toULongLong();

    // Check window identifier of tab. If matches this window's id, load the tab
    // in a new window. Otherwise, add the tab to this window
    if (static_cast<qulonglong>(winId()) == otherWinId)
    {
        MainWindow *win = sBrowserApplication->getNewWindow();
        WebWidget *webWidget = win->currentWebWidget();
        win->getTabWidget()->setTabPinned(0, webState.isPinned);
        webWidget->setHibernation(event->mimeData()->property("tab-hibernating").toBool());
        webWidget->setWebState(std::move(webState));
        win->onTabChanged(0);

        const int tabIndex = event->mimeData()->property("tab-index").toInt();
        m_tabWidget->closeTab(tabIndex);
    }
    else
    {
        auto tabBar = m_tabWidget->tabBar();
        int newTabIndex = tabBar->tabAt(event->position().toPoint());
        if (newTabIndex < 0)
            newTabIndex = tabBar->count();

        WebWidget *newTab = m_tabWidget->newTabAtIndex(newTabIndex);
        newTabIndex = m_tabWidget->indexOf(newTab);
        m_tabWidget->setTabPinned(newTabIndex, webState.isPinned);
        newTab->setHibernation(event->mimeData()->property("tab-hibernating").toBool());
        newTab->setWebState(std::move(webState));
        onTabChanged(newTabIndex);

        if (MainWindow *otherWindow = sBrowserApplication->getWindowById(static_cast<WId>(otherWinId)))
        {
            // close the tab
            const int otherTabIndex = event->mimeData()->property("tab-index").toInt();
            otherWindow->getTabWidget()->closeTab(otherTabIndex);
        }
    }

    event->acceptProposedAction();
}

void MainWindow::resizeEvent(QResizeEvent *event)
{
    QMainWindow::resizeEvent(event);
    ui->bookmarkBar->setMaximumWidth(event->size().width());
}

void MainWindow::onLinkHovered(const QString &url)
{
    if (!url.isEmpty())
    {
        QFontMetrics urlFontMetrics(m_linkHoverLabel->font());
        m_linkHoverLabel->setText(urlFontMetrics.elidedText(url, Qt::ElideRight, std::max(ui->statusBar->width() - 14, 0)));
    }
    else
        m_linkHoverLabel->setText(url);
}

void MainWindow::onSavePageTriggered()
{
    QString fileName = m_settings->getValue(BrowserSetting::DownloadDir).toString() + QDir::separator()
            + m_tabWidget->tabText(m_tabWidget->currentIndex());
    fileName = QFileDialog::getSaveFileName(this, tr("Save as..."), fileName,
                                            tr("HTML page(*.html);;MIME HTML page(*.mhtml)"));
    if (!fileName.isEmpty())
    {
        auto format = QWebEngineDownloadRequest::SingleHtmlSaveFormat;
        if (fileName.endsWith(QLatin1String("mhtml")))
            format = QWebEngineDownloadRequest::MimeHtmlSaveFormat;
        m_tabWidget->currentWebWidget()->page()->save(fileName, format);
    }
}
