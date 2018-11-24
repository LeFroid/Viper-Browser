#include "AdBlockButton.h"
#include "BrowserApplication.h"
#include "BookmarkManager.h"
#include "BookmarkNode.h"
#include "BrowserTabWidget.h"
#include "FaviconStore.h"
#include "MainWindow.h"
#include "NavigationToolBar.h"
#include "SearchEngineLineEdit.h"
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
    m_faviconStore(nullptr)
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
    m_faviconStore(nullptr)
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

void NavigationToolBar::setFaviconStore(FaviconStore *faviconStore)
{
    m_faviconStore = faviconStore;
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
    addWidget(m_prevPage);

    m_prevPage->setIcon(QIcon(QLatin1String(":/arrow-back.png")));
    m_prevPage->setToolTip(tr("Go back one page"));

    QMenu *buttonHistMenu = new QMenu(this);
    m_prevPage->setMenu(buttonHistMenu);

    connect(m_prevPage, &QToolButton::clicked, [this, win](){
        WebHistory *history = win->currentWebWidget()->getHistory();
        if (history)
            history->goBack();
    });

    // Next Page Button
    m_nextPage = new QToolButton;
    addWidget(m_nextPage);
    m_nextPage->setIcon(QIcon(QLatin1String(":/arrow-forward.png")));
    m_nextPage->setToolTip(tr("Go forward one page"));

    buttonHistMenu = new QMenu(this);
    m_nextPage->setMenu(buttonHistMenu);

    connect(m_nextPage, &QToolButton::clicked, [this, win](){
        WebHistory *history = win->currentWebWidget()->getHistory();
        if (history)
            history->goForward();
    });

    // Stop Loading / Refresh Page dual button
    m_stopRefresh = new QAction(this);
    m_stopRefresh->setIcon(QIcon(QLatin1String(":/reload.png")));
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
    QSplitter *splitter = new QSplitter(this);
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
    m_adBlockButton = new AdBlockButton;
    connect(m_adBlockButton, &AdBlockButton::clicked, this, &NavigationToolBar::clickedAdBlockButton);
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
    connect(tabWidget, &BrowserTabWidget::urlChanged, [this](const QUrl &url){
        m_urlInput->setURL(url);
    });

    connect(tabWidget, &BrowserTabWidget::currentChanged, this, &NavigationToolBar::resetHistoryButtonMenus);
    connect(tabWidget, &BrowserTabWidget::loadFinished,   this, &NavigationToolBar::resetHistoryButtonMenus);

    connect(tabWidget, &BrowserTabWidget::aboutToHibernate, [this](){
        m_nextPage->menu()->clear();
        m_prevPage->menu()->clear();
    });
}

MainWindow *NavigationToolBar::getParentWindow()
{
    return dynamic_cast<MainWindow*>(parentWidget());
}

void NavigationToolBar::onLoadProgress(int value)
{
    // Update stop/refresh icon and tooltip depending on state
    if (value > 0 && value < 100)
    {
        m_stopRefresh->setIcon(QIcon(QLatin1String(":/stop.png")));
        m_stopRefresh->setToolTip(tr("Stop loading the page"));
    }
    else
    {
        m_stopRefresh->setIcon(QIcon(QLatin1String(":/reload.png")));
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

void NavigationToolBar::resetHistoryButtonMenus()
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
    if (hist == nullptr)
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

        connect(histAction, &QAction::triggered, [=](){
            hist->goToEntry(entry);
        });
        prevAction = histAction;
    }

    // Setup forward button history menu
    histItems = hist->getForwardEntries(maxMenuSize);
    histAction = nullptr, prevAction = nullptr;
    for (const auto &entry : histItems)
    {
        histAction = forwardMenu->addAction(entry.icon, entry.title);

        connect(histAction, &QAction::triggered, [=](){
            hist->goToEntry(entry);
        });
    }
}
