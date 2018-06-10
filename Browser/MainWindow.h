#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "BookmarkManager.h"
#include "Settings.h"
#include "WebActionProxy.h"

#include <memory>
#include <QIcon>
#include <QList>
#include <QMainWindow>
#include <QWebEnginePage>

namespace Ui {
class MainWindow;
}

class QActionGroup;
class QLabel;
class QNetworkReply;
class QSslError;
class QToolButton;
class QWebInspector;
class AdBlockWidget;
class BookmarkDialog;
class BookmarkWidget;
class BrowserTabWidget;
class ClearHistoryDialog;
class Preferences;
class SearchEngineLineEdit;
class URLLineEdit;
class UserScriptWidget;
class WebView;

/**
 * @class MainWindow
 * @brief Main browser window
 */
class MainWindow : public QMainWindow
{
    friend class BrowserTabBar;
    friend class SessionManager;
    friend class URLLineEdit;
    friend class WebView;

    Q_OBJECT

public:
    /// Constructs the main window
    explicit MainWindow(std::shared_ptr<Settings> settings, BookmarkManager *bookmarkManager, bool privateWindow, QWidget *parent = 0);

    /// MainWindow destructor
    ~MainWindow();

    /// Returns true if this is a private browsing window, false if else
    bool isPrivate() const;

signals:
    /// Emitted when the window is about to be closed
    void aboutToClose();

private:
    /// Sets the proper functionality of the bookmarks menu and bookmarks bar
    void setupBookmarks();

    /// Initializes the actions belonging to the menu bar (except bookmarks, history and plugins)
    void setupMenuBar();

    /// Sets the initial state of the tab widget
    void setupTabWidget();

    /// Initializes the widgets belonging to the main tool bar
    void setupToolBar();

    /// Initializes the widgets belonging to the status bar
    void setupStatusBar();

    /// Checks if the page on the active tab is bookmarked, setting the appropriate action in
    /// the bookmark menu after checking the database
    void checkPageForBookmark();

    /// Binds the given WebAction of any active web page with a local user interface action
    inline void addWebProxyAction(QWebEnginePage::WebAction webAction, QAction *windowAction)
    {
        m_webActions.append(new WebActionProxy(webAction, windowAction, this));
    }

public slots:
    /// Called when the active tab has changed
    void onTabChanged(int index);

    /// Opens the bookmark manager
    void openBookmarkWidget();

    /// Loads the blank page into the active tab
    void loadBlankPage();

    /// Attempts to load the URL into the active tab
    void loadUrl(const QUrl &url);

    /// Attempts to load the URL into a new browsing tab
    void openLinkNewTab(const QUrl &url);

    /// Attempts to load the URL into a new window
    void openLinkNewWindow(const QUrl &url);

    /// Updates the icon for the view associated with a tab
    void updateTabIcon(QIcon icon, int tabIndex);

    /// Updates the title for the view associated with a tab
    void updateTabTitle(const QString &title, int tabIndex);

    /// Called when a URL has been entered, attempts to load the resource on the current tab's web view
    void goToURL();

    /// Launches the cookie manager UI
    void openCookieManager();

    /// Displays the download manager UI
    void openDownloadManager();

    /// Called when the clear history dialog is finished, returning a result (to cancel or clear history)
    void onClearHistoryDialogFinished(int result);

    /// Refreshes the bookmarks menu
    void refreshBookmarkMenu();

    /// Attempts to add the current page to the user's bookmark collection
    void addPageToBookmarks();

    /// Attempts to remove the current page from the user's bookmarks
    void removePageFromBookmarks(bool showDialog);

    /// Toggles visibility of the bookmark bar
    void toggleBookmarkBar(bool enabled);

    /// Shows the find text widget for the web view, if currently hidden
    void onFindTextAction();

private slots:
    /// Launches the ad block manager UI
    void openAdBlockManager();

    /// Launches the dialog used to clear browsing history and storage
    void openClearHistoryDialog();

    /// Launches the browser settings / preferences UI
    void openPreferences();

    /// Launches the user script manager UI
    void openUserScriptManager();

    /// Spawns a file chooser dialog so the user can open a file into the web browser
    void openFileInBrowser();

    /// Called when the tab widget signals a page has progressed in loading its content
    void onLoadProgress(int value);

    /// Called when a page is finished loading. If 'ok' is false, there was an error while loading the page
    void onLoadFinished(bool ok);

    /// Called when the user requests to open the history view widget
    void onShowAllHistory();

    /// Connects the signals emitted by the web view to slots handled by the browser window
    void onNewTabCreated(WebView *view);

    /// Called when the user wishes to view the certificate information (or lack thereof) for the current web page
    void onClickSecurityInfo();

    /// Called when the user wants to view the source code of the current web page
    void onRequestViewSource();

    /**
     * @brief onToggleFullScreen Toggles the full screen view mode of the browser window
     * @param enable If true, will activate the full screen view. Otherwise, will return to normal view mode
     */
    void onToggleFullScreen(bool enable);

    /// Called when the user requests that the contents of the current browser tab be printed
    void printTabContents();

    /// Called when the bookmark icon is clicked by the user
    void onClickBookmarkIcon();

    /// Called when a link is hovered by the user
    void onLinkHovered(const QUrl &url);

protected slots:
    /**
     * @brief Called by a \ref WebView when it is requested that some content be opened in a new tab.
     * @param makeCurrent If true, the tab widget will switch its active tab to the newly created view.
     * @return A pointer to the new WebView.
     */
    WebView *getNewTabWebView(bool makeCurrent);

protected:
    /// Returns a pointer to the tab widget. Used by \ref SessionManager to save the browsing session
    BrowserTabWidget *getTabWidget() const;

    /// Returns the height of the toolbar. Used by \ref URLLineEdit for its size hint
    int getToolbarHeight() const;

    /// Called when the window is being closed
    void closeEvent(QCloseEvent *event) override;

    /// Handles drag enter events (supports tab drags)
    void dragEnterEvent(QDragEnterEvent *event) override;

    /// Handles tab drop events
    void dropEvent(QDropEvent *event) override;

private:
    /// UI items from .ui file
    Ui::MainWindow *ui;

    /// True if window is a private browsing window, false if else
    bool m_privateWindow;

    /// Shared pointer to browser settings
    std::shared_ptr<Settings> m_settings;

    /// Bookmark manager
    BookmarkManager *m_bookmarkManager;

    /// Bookmark manager interface
    BookmarkWidget *m_bookmarkUI;

    /// Dialog used to clear recent history, cookies, etc.
    ClearHistoryDialog *m_clearHistoryDialog;

    /// Browser tab widget
    BrowserTabWidget *m_tabWidget;

    /// List of web action proxies that bind menu bar actions to web page actions
    QList<WebActionProxy*> m_webActions;

    /// Button to go to the previously visited page
    QToolButton *m_prevPage;

    /// Action to return to the next page after visiting a previous page
    QToolButton *m_nextPage;

    /// Action to either stop the loading progress of a page, or to refresh the loaded page
    QAction *m_stopRefresh;

    /// URL Input bar
    URLLineEdit *m_urlInput;

    /// Search engine line edit, located in the browser toolbar
    SearchEngineLineEdit *m_searchEngineLineEdit;

    /// Preferences window
    Preferences *m_preferences;

    /// Dialog that is shown when a bookmark is added or is being modified by the user
    BookmarkDialog *m_bookmarkDialog;

    /// User script management widget
    UserScriptWidget *m_userScriptWidget;

    /// Advertisement blocking management widget
    AdBlockWidget *m_adBlockWidget;

    /// Stores javascript code that attempts to extract favicon path from a page
    QString m_faviconScript;

    /// Displays the link being hovered by the user in the status bar
    QLabel *m_linkHoverLabel;
};

#endif // MAINWINDOW_H
