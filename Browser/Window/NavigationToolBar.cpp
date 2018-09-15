#include "AdBlockButton.h"
#include "BrowserApplication.h"
#include "BookmarkManager.h"
#include "BookmarkNode.h"
#include "BrowserTabWidget.h"
#include "MainWindow.h"
#include "NavigationToolBar.h"
#include "SearchEngineLineEdit.h"
#include "URLLineEdit.h"
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
    m_adBlockButton(nullptr)
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
    m_adBlockButton(nullptr)
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

void NavigationToolBar::setupUI()
{
    MainWindow *win = getParentWindow();
    if (!win)
        return;

    setFloatable(false);

    // Previous Page Button
    m_prevPage = new QToolButton;
    QAction *prevPageAction = addWidget(m_prevPage);

    m_prevPage->setIcon(style()->standardIcon(QStyle::SP_ArrowBack, 0, this));
    m_prevPage->setToolTip(tr("Go back one page"));

    QMenu *buttonHistMenu = new QMenu(this);
    m_prevPage->setMenu(buttonHistMenu);

    connect(m_prevPage, &QToolButton::clicked, prevPageAction, &QAction::trigger);
    win->addWebProxyAction(QWebEnginePage::Back, prevPageAction);

    // Next Page Button
    m_nextPage = new QToolButton;
    QAction *nextPageAction = addWidget(m_nextPage);

    m_nextPage->setIcon(style()->standardIcon(QStyle::SP_ArrowForward, 0, this));
    m_nextPage->setToolTip(tr("Go forward one page"));

    buttonHistMenu = new QMenu(this);
    m_nextPage->setMenu(buttonHistMenu);

    connect(m_nextPage, &QToolButton::clicked, nextPageAction, &QAction::trigger);
    win->addWebProxyAction(QWebEnginePage::Forward, nextPageAction);

    // Stop Loading / Refresh Page dual button
    m_stopRefresh = new QAction(this);
    m_stopRefresh->setIcon(style()->standardIcon(QStyle::SP_BrowserReload));
    connect(m_stopRefresh, &QAction::triggered, this, &NavigationToolBar::onStopRefreshActionTriggered);

    // URL Bar
    m_urlInput = new URLLineEdit(win);
    connect(m_urlInput, &URLLineEdit::returnPressed, this, &NavigationToolBar::onURLInputEntered);
    connect(m_urlInput, &URLLineEdit::viewSecurityInfo, win, &MainWindow::onClickSecurityInfo);
    connect(m_urlInput, &URLLineEdit::toggleBookmarkStatus, win, &MainWindow::onClickBookmarkIcon);

    // Quick search tool
    m_searchEngineLineEdit = new SearchEngineLineEdit(win);
    m_searchEngineLineEdit->setFont(m_urlInput->font());
    connect(m_searchEngineLineEdit, &SearchEngineLineEdit::requestPageLoad, win, &MainWindow::loadUrl);
    connect(m_searchEngineLineEdit, &SearchEngineLineEdit::requestPageLoadPost, win, &MainWindow::loadHttpRequest);

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

    // Pass pointers to previous and next page menus to the tab widget
    tabWidget->setNavHistoryMenus(m_prevPage->menu(), m_nextPage->menu());
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
        m_stopRefresh->setIcon(style()->standardIcon(QStyle::SP_BrowserStop));
        m_stopRefresh->setToolTip(tr("Stop loading the page"));
    }
    else
    {
        m_stopRefresh->setIcon(style()->standardIcon(QStyle::SP_BrowserReload));
        m_stopRefresh->setToolTip(tr("Reload the page"));
    }

    //m_adBlockButton->updateCount();
}

void NavigationToolBar::onURLInputEntered()
{
    MainWindow *win = getParentWindow();
    if (!win)
        return;

    WebWidget *view = win->currentWebWidget();
    if (!view)
        return;

    QString urlText = m_urlInput->text();
    if (urlText.isEmpty())
        return;

    QUrl location = QUrl::fromUserInput(urlText);
    if (location.isValid() && !location.topLevelDomain().isNull())
    {
        view->load(location);
        view->setFocus();
        m_urlInput->setText(location.toString(QUrl::FullyEncoded));
    }
    else
    {
        QString urlTextStart = urlText;
        int delimIdx = urlTextStart.indexOf(QLatin1Char(' '));
        if (delimIdx > 0)
            urlTextStart = urlTextStart.left(delimIdx);

        BookmarkManager *bookmarkMgr = sBrowserApplication->getBookmarkManager();
        for (auto it : *bookmarkMgr)
        {
            if (it->getType() == BookmarkNode::Bookmark
                    && urlTextStart.compare(it->getShortcut()) == 0)
            {
                if (delimIdx > 0)
                {
                    QString temp = it->getURL();
                    urlText = temp.replace(QLatin1String("%s"), urlText.mid(delimIdx + 1));
                }
                else
                    urlText = it->getURL();

                location = QUrl::fromUserInput(urlText);
                view->load(location);
                view->setFocus();
                m_urlInput->setText(location.toString(QUrl::FullyEncoded));
                return;
            }
        }

        view->load(location);
        view->setFocus();
        m_urlInput->setText(location.toString(QUrl::FullyEncoded));
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
