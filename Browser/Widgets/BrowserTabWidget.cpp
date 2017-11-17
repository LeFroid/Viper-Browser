#include "BrowserApplication.h"
#include "BrowserTabWidget.h"
#include "BrowserTabBar.h"
#include "MainWindow.h"
#include "WebView.h"

BrowserTabWidget::BrowserTabWidget(std::shared_ptr<Settings> settings, QWidget *parent) :
    QTabWidget(parent),
    m_settings(settings),
    m_privateBrowsing(false),
    m_newTabPage(settings->getValue("NewTabsLoadHomePage").toBool() ? HomePage : BlankPage),
    m_activeView(nullptr),
    m_tabBar(new BrowserTabBar(this))
{
    // Set tab bar
    setTabBar(m_tabBar);

    // Set tab widget UI properties
    setDocumentMode(true);
    setElideMode(Qt::ElideRight);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    connect(this, &BrowserTabWidget::tabCloseRequested, this, &BrowserTabWidget::closeTab);
    connect(this, &BrowserTabWidget::currentChanged, this, &BrowserTabWidget::onCurrentChanged);

    connect(m_tabBar, &BrowserTabBar::newTabRequest, [=](){ newTab(); });
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

void BrowserTabWidget::setPrivateMode(bool value)
{
    if (m_privateBrowsing == value)
        return;

    m_privateBrowsing = value;

    int numViews = count();
    for (int i = 0; i < numViews; ++i)
    {
        if (m_tabBar->isTabEnabled(i))
            getWebView(i)->setPrivate(value);
    }
}

void BrowserTabWidget::closeTab(int index)
{
    int numTabs = count();
    if (index < 0 || numTabs == 1 || index >= numTabs)
        return;

    WebView *view = getWebView(index);
    view->deleteLater();
    removeTab(index);
}

WebView *BrowserTabWidget::newTab(bool makeCurrent, bool skipHomePage)
{
    WebView *view = new WebView(parentWidget());
    view->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

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
    connect(view, &WebView::iconChanged, this, &BrowserTabWidget::onIconChanged);
    connect(view, &WebView::openRequest, this, &BrowserTabWidget::loadUrl);
    connect(view, &WebView::openInNewTabRequest, this, &BrowserTabWidget::openLinkInNewTab);
    connect(view, &WebView::openInNewWindowRequest, this, &BrowserTabWidget::openLinkInNewWindow);

    addTab(view, tabLabel);
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
    setTabIcon(tabIndex, QWebSettings::iconForUrl(view->url()));
}


void BrowserTabWidget::openLinkInNewTab(const QUrl &url)
{
    // Create view, load home page, add view to tab widget
    WebView *view = newTab(false, true);
    view->load(url);
}

void BrowserTabWidget::openLinkInNewWindow(const QUrl &url, bool privateWindow)
{
    MainWindow *newWin = sBrowserApplication->getNewWindow();
    if (!newWin)
        return;
    newWin->setPrivate(privateWindow);
    newWin->loadUrl(url);
}

void BrowserTabWidget::loadUrl(const QUrl &url)
{
    currentWebView()->load(url);
}

void BrowserTabWidget::onCurrentChanged(int index)
{
    WebView *view = getWebView(index);
    if (!view)
        return;

    // Disconnect loadProgress signal from previously active web view if applicable
    if (m_activeView && count() > 1)
        disconnect(m_activeView, &WebView::loadProgress, this, &BrowserTabWidget::loadProgress);

    m_activeView = view;
    connect(view, &WebView::loadProgress, this, &BrowserTabWidget::loadProgress);

    emit loadProgress(view->getProgress());
    emit viewChanged(index);
}
