#include "AdBlockButton.h"
#include "AdBlockManager.h"
#include "BookmarkManager.h"
#include "BookmarkNode.h"
#include "BrowserApplication.h"
#include "BrowserTabWidget.h"
#include "FaviconManager.h"
#include "MainWindow.h"
#include "NavigationToolBar.h"
#include "SearchEngineLineEdit.h"
#include "Settings.h"
#include "URLLineEdit.h"
#include "WebHistory.h"
#include "WebPage.h"
#include "WebView.h"
#include "WebWidget.h"

#include <QMenu>
#include <QSplitter>
#include <QStyle>
#include <QToolButton>

NavigationToolBar::NavigationToolBar(const QString &title, QWidget *parent) :
    QToolBar(title, parent),
    m_prevPage(nullptr),
    m_nextPage(nullptr),
    m_stopRefresh(nullptr),
    m_urlInput(nullptr),
    m_searchEngineLineEdit(nullptr),
    m_splitter(nullptr),
    m_adBlockButton(nullptr),
    m_isDarkTheme(false)
{
    setupUI();
}

NavigationToolBar::NavigationToolBar(QWidget *parent) :
    QToolBar(parent),
    m_prevPage(nullptr),
    m_nextPage(nullptr),
    m_stopRefresh(nullptr),
    m_urlInput(nullptr),
    m_searchEngineLineEdit(nullptr),
    m_splitter(nullptr),
    m_adBlockButton(nullptr),
    m_isDarkTheme(false)
{
    setupUI();
}

NavigationToolBar::~NavigationToolBar()
{
}

SearchEngineLineEdit *NavigationToolBar::getSearchEngineWidget()
{
    return m_searchEngineLineEdit;
}

URLLineEdit *NavigationToolBar::getURLWidget()
{
    return m_urlInput;
}

void NavigationToolBar::setMinHeights(int size)
{
    if (size <= 12)
        return;

    setMinimumHeight(size);

    const int subWidgetMinHeight = size - 12;
    m_prevPage->setMinimumHeight(subWidgetMinHeight);
    m_nextPage->setMinimumHeight(subWidgetMinHeight);
    m_urlInput->setMinimumHeight(subWidgetMinHeight);
    m_searchEngineLineEdit->setMinimumHeight(subWidgetMinHeight);
    m_adBlockButton->setMinimumHeight(subWidgetMinHeight);
}

void NavigationToolBar::setServiceLocator(const ViperServiceLocator &serviceLocator)
{
    m_searchEngineLineEdit->setFaviconManager(serviceLocator.getServiceAs<FaviconManager>("FaviconManager"));

    m_adBlockButton->setAdBlockManager(serviceLocator.getServiceAs<adblock::AdBlockManager>("AdBlockManager"));
    m_adBlockButton->setSettings(serviceLocator.getServiceAs<Settings>("Settings"));
    m_urlInput->setServiceLocator(serviceLocator);
}

void NavigationToolBar::setupUI()
{
    MainWindow *win = getParentWindow();
    if (!win)
        return;

    setFloatable(false);
    setObjectName(QLatin1String("navigationToolBar"));

    // Previous Page Button
    m_prevPage = new QToolButton;
    QAction *pageAction = addWidget(m_prevPage);

    m_isDarkTheme = sBrowserApplication->isDarkTheme();
    m_prevPage->setIcon(QIcon(m_isDarkTheme ? QStringLiteral(":/arrow-back-white.png") : QStringLiteral(":/arrow-back.png")));
    m_prevPage->setToolTip(tr("Go back one page"));

    QMenu *buttonHistMenu = new QMenu(this);
    m_prevPage->setMenu(buttonHistMenu);

    connect(m_prevPage, &QToolButton::clicked, [win](){
        WebHistory *history = win->currentWebWidget()->getHistory();
        if (history)
            history->goBack();
    });
    win->addWebProxyAction(WebPage::Back, pageAction);

    // Next Page Button
    m_nextPage = new QToolButton;
    pageAction = addWidget(m_nextPage);
    m_nextPage->setIcon(QIcon(m_isDarkTheme ? QStringLiteral(":/arrow-forward-white.png") : QStringLiteral(":/arrow-forward.png")));
    m_nextPage->setToolTip(tr("Go forward one page"));

    buttonHistMenu = new QMenu(this);
    m_nextPage->setMenu(buttonHistMenu);
    connect(m_nextPage, &QToolButton::clicked, [win](){
        WebHistory *history = win->currentWebWidget()->getHistory();
        if (history)
            history->goForward();
    });
    win->addWebProxyAction(WebPage::Forward, pageAction);

    // Stop Loading / Refresh Page dual button
    m_stopRefresh = new QAction(this);

    m_stopRefresh->setIcon(QIcon(m_isDarkTheme ? QStringLiteral(":/reload-white.png") : QStringLiteral(":/reload.png")));
    connect(m_stopRefresh, &QAction::triggered, this, &NavigationToolBar::onStopRefreshActionTriggered);

    // URL Bar
    m_urlInput = new URLLineEdit(win);
    connect(m_urlInput, &URLLineEdit::viewSecurityInfo, win, &MainWindow::onClickSecurityInfo);
    connect(m_urlInput, &URLLineEdit::toggleBookmarkStatus, win, &MainWindow::onClickBookmarkIcon);

    // Quick search tool
    m_searchEngineLineEdit = new SearchEngineLineEdit(win);
    m_searchEngineLineEdit->setFont(m_urlInput->font());
    connect(m_searchEngineLineEdit, &SearchEngineLineEdit::requestPageLoad, win, &MainWindow::loadHttpRequest);

    // Splitter for resizing URL bar and quick search bar
    QSplitter *splitter = new QSplitter(Qt::Horizontal, this);
    splitter->addWidget(m_urlInput);
    splitter->addWidget(m_searchEngineLineEdit);

    // Reduce height of URL bar and quick search
    int lineEditHeight = height() * 2 / 3 + 1;
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

    addAction(m_stopRefresh);
    addWidget(splitter);

    // Ad block button
    m_adBlockButton = new AdBlockButton(m_isDarkTheme);
    connect(m_adBlockButton, &AdBlockButton::clicked, this, &NavigationToolBar::clickedAdBlockButton);
    connect(m_adBlockButton, &AdBlockButton::viewLogsRequest, this, &NavigationToolBar::clickedAdBlockButton);
    connect(m_adBlockButton, &AdBlockButton::manageSubscriptionsRequest, this, &NavigationToolBar::requestManageAdBlockSubscriptions);
    addWidget(m_adBlockButton);
}

void NavigationToolBar::bindWithTabWidget()
{
    MainWindow *win = getParentWindow();
    if (!win)
        return;

    // Remove a mapped webview from the url bar when it is going to be deleted
    BrowserTabWidget *tabWidget = win->getTabWidget();
    connect(tabWidget, &BrowserTabWidget::tabClosing, m_urlInput, &URLLineEdit::removeMappedView);

    // Page load progress handler
    connect(tabWidget, &BrowserTabWidget::loadProgress, this, &NavigationToolBar::onLoadProgress);

    // URL change in current tab
    connect(tabWidget, &BrowserTabWidget::urlChanged, m_urlInput, &URLLineEdit::setURL);

    connect(tabWidget, &BrowserTabWidget::currentChanged, this, &NavigationToolBar::onHistoryChanged);
    connect(tabWidget, &BrowserTabWidget::loadFinished, this, &NavigationToolBar::onHistoryChanged);

    connect(tabWidget, &BrowserTabWidget::aboutToHibernate, [this](){
        m_nextPage->menu()->clear();
        m_prevPage->menu()->clear();
    });
    connect(tabWidget, &BrowserTabWidget::aboutToWake, [this, tabWidget]() {
        WebHistory *hist = tabWidget->currentWebWidget()->getHistory();
        if (hist)
            connect(hist, &WebHistory::historyChanged, this, &NavigationToolBar::onHistoryChanged, Qt::UniqueConnection);
    });
}

MainWindow *NavigationToolBar::getParentWindow()
{
    return qobject_cast<MainWindow*>(window());
}

void NavigationToolBar::onTabChanged(int index)
{
    onHistoryChanged();

    BrowserTabWidget *tabWidget = qobject_cast<BrowserTabWidget*>(sender());
    if (!tabWidget)
        return;

    WebWidget *webWidget = tabWidget->getWebWidget(index);
    if (!webWidget)
        return;

    WebHistory *hist = webWidget->getHistory();
    if (!hist)
        return;

    connect(hist, &WebHistory::historyChanged, this, &NavigationToolBar::onHistoryChanged, Qt::UniqueConnection);
}

void NavigationToolBar::onLoadProgress(int value)
{
    // Update stop/refresh icon and tooltip depending on state
    if (value > 0 && value < 100)
    {
        m_stopRefresh->setIcon(QIcon(m_isDarkTheme ? QStringLiteral(":/stop-white.png") : QStringLiteral(":/stop.png")));
        m_stopRefresh->setToolTip(tr("Stop loading the page"));
    }
    else
    {
        m_stopRefresh->setIcon(QIcon(m_isDarkTheme ? QStringLiteral(":/reload-white.png") : QStringLiteral(":/reload.png")));
        m_stopRefresh->setToolTip(tr("Reload the page"));
    }
}

void NavigationToolBar::onStopRefreshActionTriggered()
{
    MainWindow *win = getParentWindow();
    if (!win)
        return;

    if (WebWidget *ww = win->currentWebWidget())
    {
        int progress = ww->getProgress();
        if (progress > 0 && progress < 100)
            ww->stop();
        else
            ww->reload();
    }
}

void NavigationToolBar::onHistoryChanged()
{
    QMenu *backMenu = m_prevPage->menu();
    QMenu *forwardMenu = m_nextPage->menu();

    backMenu->clear();
    forwardMenu->clear();

    MainWindow *win = getParentWindow();
    if (!win)
        return;

    WebWidget *webWidget = win->currentWebWidget();
    if (!webWidget)
        return;

    WebHistory *hist = webWidget->getHistory();
    WebHistory *caller = qobject_cast<WebHistory*>(sender());

    if (webWidget->isHibernating() || hist == nullptr || (caller != nullptr && caller != hist))
    {
        m_prevPage->setEnabled(false);
        m_nextPage->setEnabled(false);
        return;
    }

    m_prevPage->setEnabled(hist->canGoBack());
    m_nextPage->setEnabled(hist->canGoForward());

    const int maxMenuSize = 10;
    QAction *histAction = nullptr, *prevAction = nullptr;

    // Setup back button history menu
    auto histItems = hist->getBackEntries(maxMenuSize);
    for (const auto &entry : histItems)
    {
        if (prevAction == nullptr)
            histAction = backMenu->addAction(entry.icon, entry.title);
        else
        {
            histAction = new QAction(entry.icon, entry.title, backMenu);
            backMenu->insertAction(prevAction, histAction);
        }

        connect(histAction, &QAction::triggered, backMenu, [hist, entry](){
            hist->goToEntry(entry);
        });
        prevAction = histAction;
    }

    // Setup forward button history menu
    histItems = hist->getForwardEntries(maxMenuSize);
    histAction = nullptr;
    prevAction = nullptr;
    for (const auto &entry : histItems)
    {
        histAction = forwardMenu->addAction(entry.icon, entry.title);

        connect(histAction, &QAction::triggered, forwardMenu, [hist, entry](){
            hist->goToEntry(entry);
        });
    }
}
