#ifndef BROWSERTABWIDGET_H
#define BROWSERTABWIDGET_H

#include "Settings.h"

#include <deque>
#include <memory>
#include <QTabWidget>
#include <QUrl>

class BrowserTabBar;
class MainWindow;
class QMenu;
class WebView;

/**
 * @struct ClosedTabInfo
 * @brief Contains information about a tab that was closed, used to restore a tab to
 *        its prior state.
 */
struct ClosedTabInfo
{
    /// Index of the tab in the tab bar
    int index;

    /// Last URL loaded into the tab's WebView
    QUrl url;

    /// Stores the page history of the tab
    QByteArray pageHistory;

    /// Default constructor
    ClosedTabInfo() = default;

    /// Constructs the ClosedTabInfo given a tab index and a pointer to the tab's WebView
    ClosedTabInfo(int tabIndex, WebView *view);
};

/**
 * @class BrowserTabWidget
 * @brief Handles rendering of browser tabs containing WebView widgets
 */
class BrowserTabWidget : public QTabWidget
{
    Q_OBJECT

    /// Which page to load by default when a new tab is created
    enum NewTabPage { HomePage = 0, BlankPage = 1 };

public:
    /// Constructs the browser tab widget with the given parent
    BrowserTabWidget(std::shared_ptr<Settings> settings, bool privateMode, QWidget *parent = nullptr);

    /// Returns a pointer to the current web view
    WebView *currentWebView() const;

    /// Returns the web view at the given tab index, or a nullptr if tabIndex is invalid
    WebView *getWebView(int tabIndex) const;

    /// Filters events for the watched object
    bool eventFilter(QObject *watched, QEvent *event);

    /// Returns true if at least one tab has been closed which can be reopened, false if else
    bool canReopenClosedTab() const;

    /// Returns true if the tab at the given index is pinned, false if else
    bool isTabPinned(int tabIndex) const;

signals:
    /// Emitted when a new tab is created, passing along a pointer to the associated web view
    void newTabCreated(WebView *view);

    /// Emitted when a tab containing the given view is being closed
    void tabClosing(WebView *view);

    /// Emitted when the current view has made progress loading its page. The range of value is [0,100]
    void loadProgress(int value);

    /// Emitted when the active tab / web view has changed
    void viewChanged(int index);

public slots:
    /// Sets the tab at the given index to be pinned if value is true, otherwise it will be unpinned
    void setTabPinned(int index, bool value);

    /// Reopens the last tab that was closed
    void reopenLastTab();

    /// Called when a tab is to be closed
    void closeTab(int index = -1);

    /// Creates a new duplicate of the tab at the given index
    void duplicateTab(int index);

    /**
     * @brief Creates a new tab, assigning a WebView as its associated widget.
     * @param makeCurrent If true, the TabWidget's active tab will be switched to this new tab
     * @param skipHomePage If true, the home page will not be loaded.
     * @param specificIndex If the given index is >= 0, the tab will be inserted at that index
     * @return A pointer to the tab's WebView
     */
    WebView *newTab(bool makeCurrent = false, bool skipHomePage = false, int specificIndex = -1);

    /// Called when the icon for a web view has changed
    void onIconChanged();

    /// Spawns a new browser tab, loading the given URL
    void openLinkInNewTab(const QUrl &url, bool makeCurrent = false);

    /// Opens the given link in a new browser window, and sets the window to private mode if requested
    void openLinkInNewWindow(const QUrl &url, bool privateWindow);

    /// Loads the given URL in the active tab
    void loadUrl(const QUrl &url);

    /// Sets the back button and forward button history menus when a tab is changed
    void setNavHistoryMenus(QMenu *backMenu, QMenu *forwardMenu);

    /// Resets the zoom factor of the active tab's \ref WebView to its base value
    void resetZoomCurrentView();

    /// Increases the zoom factor of the active tab's \ref WebView by 10% of the base value
    void zoomInCurrentView();

    /// Decreases the zoom factor of the active tab's \ref WebView by 10% of the base value
    void zoomOutCurrentView();

private slots:
    /// Called when the current tab has been changed
    void onCurrentChanged(int index);

    /// Called by a web view when it has made progress loading a page
    void onLoadProgress(int progress);

    /// Called when a web page's title has changed to the given value
    void onTitleChanged(const QString &title);

    /// Emitted when a view requests that it be closed
    void onViewCloseRequested();

    /// Resets the items in the back and forward button menus, populating them with the current tab's history
    void resetHistoryButtonMenus(bool ok);

    /// Displays a context menu for the current web view
    void showContextMenuForView();

private:
    /// Saves the tab at the given index before closing it
    void saveTab(int index);

private:
    /// Browser settings
    std::shared_ptr<Settings> m_settings;

    /// Private browsing flag
    bool m_privateBrowsing;

    /// Active web view
    WebView *m_activeView;

    /// Custom tab bar
    BrowserTabBar *m_tabBar;

    /// Back button menu, from the \ref MainWindow toolbar
    QMenu *m_backMenu;

    /// Forward button menu, from the \ref MainWindow toolbar
    QMenu *m_forwardMenu;

    /// Stores the index of the last two active tabs
    int m_lastTabIndex, m_currentTabIndex;

    /// Stores the index of the next tab to be created. Starts at current index + 1, and increments with
    /// each new tab created. Resets on change of active tab
    int m_nextTabIndex;

    /// Global and relative positions of the requested context menu from the active web view
    QPoint m_contextMenuPosGlobal, m_contextMenuPosRelative;

    /// Pointer to the window containing this widget
    MainWindow *m_mainWindow;

    /// Maintains a record of up to 30 tabs that were closed within the tab widget
    std::deque<ClosedTabInfo> m_closedTabs;
};

#endif // BROWSERTABWIDGET_H
