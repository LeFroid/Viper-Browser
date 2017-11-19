#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "BookmarkManager.h"
#include "Settings.h"
#include "WebActionProxy.h"

#include <memory>
#include <QIcon>
#include <QList>
#include <QMainWindow>
#include <QWebPage>

namespace Ui {
class MainWindow;
}

class QActionGroup;
class QNetworkReply;
class QSslError;
class QToolButton;
class QWebInspector;
class AddBookmarkDialog;
class BookmarkWidget;
class BrowserTabWidget;
class ClearHistoryDialog;
class CookieWidget;
class Preferences;
class SearchEngineLineEdit;
class URLLineEdit;
class WebView;

/**
 * @class MainWindow
 * @brief Main browser window
 */
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    /// Constructs the main window
    explicit MainWindow(std::shared_ptr<Settings> settings, std::shared_ptr<BookmarkManager> bookmarkManager, QWidget *parent = 0);

    /// MainWindow destructor
    ~MainWindow();

    /// Sets the browsing mode to private if flag is true, otherwise disables private browsing mode
    void setPrivate(bool value);

    /// Adds an item to the history menu
    void addHistoryItem(const QUrl &url, const QString &title, const QIcon &favicon);

    /// Removes the items contained in the history menu
    void clearHistoryItems();

    /// Resets the contents of the user agent sub-menu of the tools menu
    void resetUserAgentMenu();

private:
    /// Instantiates the items belonging to the bookmarks menu
    void setupBookmarks();

    /// Recursively initializes the items belonging to the given bookmark folder, placing them into the menu
    void setupBookmarkFolder(BookmarkNode *folder, QMenu *folderMenu);

    /// Initializes the actions belonging to the menu bar (except bookmarks, history and plugins)
    void setupMenuBar();

    /// Sets the initial state of the tab widget
    void setupTabWidget();

    /// Initializes the widgets belonging to the main tool bar
    void setupToolBar();

    /// Checks if the page on the active tab is bookmarked, setting the appropriate action in
    /// the bookmark menu after checking the database
    void checkPageForBookmark();

    /// Binds the given WebAction of any active web page with a local user interface action
    inline void addWebProxyAction(QWebPage::WebAction webAction, QAction *windowAction)
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
    void removePageFromBookmarks();

    /// Toggles visibility of the bookmark bar
    void toggleBookmarkBar(bool enabled);

    /// Shows the find text widget for the web view, if currently hidden
    void onFindTextAction();

private slots:
    /// Spawns a file chooser dialog so the user can open a file into the web browser
    void openFileInBrowser();

    /// Called when the tab widget signals a page has progressed in loading its content
    void onLoadProgress(int value);

    /// Called when a page is finished loading. If 'ok' is false, there was an error while loading the page
    void onLoadFinished(WebView *view, bool ok);

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

private:
    /// UI items from .ui file
    Ui::MainWindow *ui;

    /// True if window is a private browsing window, false if else
    bool m_privateWindow;

    /// Shared pointer to browser settings
    std::shared_ptr<Settings> m_settings;

    /// Bookmark manager
    std::shared_ptr<BookmarkManager> m_bookmarkManager;

    /// Bookmark manager interface
    BookmarkWidget *m_bookmarkUI;

    /// Dialog used to clear recent history, cookies, etc.
    ClearHistoryDialog *m_clearHistoryDialog;

    /// Cookie manager interface
    CookieWidget *m_cookieUI;

    /// Browser tab widget
    BrowserTabWidget *m_tabWidget;

    /// List of web action proxies that bind menu bar actions to web page actions
    QList<WebActionProxy*> m_webActions;

    /// Action to add the current page to the bookmarks
    QAction *m_addPageBookmarks;

    /// Action to remove the current page from the bookmarks
    QAction *m_removePageBookmarks;

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

    /// Action group for user agent items in the menu bar
    QActionGroup *m_userAgentGroup;

    /// Preferences window
    Preferences *m_preferences;

    /// Dialog that is shown when a bookmark is added through the bookmark menu option
    AddBookmarkDialog *m_addBookmarkDialog;
};

#endif // MAINWINDOW_H
