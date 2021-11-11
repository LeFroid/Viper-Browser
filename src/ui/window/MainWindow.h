#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "BookmarkManager.h"
#include "ServiceLocator.h"
#include "Settings.h"
#include "WebActionProxy.h"

#include <atomic>
#include <memory>
#include <unordered_map>

#include <QEventLoop>
#include <QIcon>
#include <QList>
#include <QPrinter>
#include <QMainWindow>
#include <QWebEnginePage>

namespace Ui {
class MainWindow;
}

class QActionGroup;
class QLabel;
class QNetworkReply;
class QToolButton;
class QWebInspector;
class QPrinter;

class BookmarkDialog;
class BookmarkWidget;
class BrowserTabWidget;
class ClearHistoryDialog;
class HttpRequest;
class SearchEngineLineEdit;
class URLLineEdit;
class WebView;
class WebWidget;

/**
 * @class MainWindow
 * @brief Main browser window
 */
class MainWindow : public QMainWindow
{
    friend class BrowserTabBar;
    friend class BrowserTabWidget;
    friend class NavigationToolBar;
    friend class SessionManager;
    friend class TabBarMimeDelegate;
    friend class URLLineEdit;
    friend class WebPage;
    friend class WebView;
    friend class WebWidget;

    Q_OBJECT

public:
    /// Constructs a browser window
    explicit MainWindow(const ViperServiceLocator &serviceLocator, bool privateWindow, QWidget *parent = nullptr);

    /// MainWindow destructor
    ~MainWindow();

    /// Returns true if this is a private browsing window, false if else
    bool isPrivate() const;

    /// Returns a pointer to the current web widget
    WebWidget *currentWebWidget() const;

    /// Informs the window that the application is about to exit
    void prepareToClose();

Q_SIGNALS:
    /// Emitted when the window is about to be closed
    void aboutToClose();

protected:
    /// Binds the given WebAction of any active web page with a local user interface action
    inline void addWebProxyAction(WebPage::WebAction webAction, QAction *windowAction)
    {
        m_webActions.append(new WebActionProxy(webAction, windowAction, this));
    }

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

public Q_SLOTS:
    /// Called when the active tab has changed
    void onTabChanged(int index);

    /// Opens the bookmark manager
    void openBookmarkWidget();

    /// Loads the blank page into the active tab
    void loadBlankPage();

    /// Attempts to load the URL into the active tab
    void loadUrl(const QUrl &url);

    /// Loads the HTTP request into the active web widget
    void loadHttpRequest(const HttpRequest &request);

    /// Attempts to load the URL into a new browsing tab
    void openLinkNewTab(const QUrl &url);

    /// Attempts to load the URL into a new window
    void openLinkNewWindow(const QUrl &url);

    /// Called when the clear history dialog is finished, returning a result (to cancel or clear history)
    void onClearHistoryDialogFinished(int result);

    /// Attempts to add the current page to the user's bookmark collection
    void addPageToBookmarks();

    /// Attempts to remove the current page from the user's bookmarks
    void removePageFromBookmarks(bool showDialog);

    /// Toggles visibility of the bookmark bar
    void toggleBookmarkBar(bool enabled);

    /// Shows the find text widget for the web view, if currently hidden
    void onFindTextAction();

protected Q_SLOTS:
    /// Called when the bookmark icon is clicked by the user
    void onClickBookmarkIcon();

    /// Called when the user wishes to view the certificate information (or lack thereof) for the current web page
    void onClickSecurityInfo();

    /**
     * @brief onToggleFullScreen Toggles the full screen view mode of the browser window
     * @param enable If true, will activate the full screen view. Otherwise, will return to normal view mode
     */
    void onToggleFullScreen(bool enable);

    /// Handles the mouse move event when in fullscreen mode
    void onMouseMoveFullscreen(int y);

    /// Opens and/or creates the web inspector, set to the current page
    void openInspector();

private Q_SLOTS:
    /// Checks if the page on the active tab is bookmarked, setting the appropriate action in
    /// the bookmark menu after checking the database
    void checkPageForBookmark();

    /// Opens the ad block log event viewer
    void openAdBlockLogDisplay();

    /// Launches the saved credentials window, showing the logins that are stored in the
    /// \ref CredentialStore and used by \ref AutoFill (if enabled)
    void openAutoFillCredentialsView();

    /// Launches the exempt login window, showing a list of website-username combinations
    /// that the \ref AutoFill system is forbidden from saving or using.
    void openAutoFillExceptionsView();

    /// Launches the dialog used to clear browsing history and storage
    void openClearHistoryDialog();

    /// Launches the browser settings / preferences UI
    void openPreferences();

    /// Spawns a file chooser dialog so the user can open a file into the web browser
    void openFileInBrowser();

    /// Called when a page is finished loading. If 'ok' is false, there was an error while loading the page
    void onLoadFinished(bool ok);

    /// Called when the user requests to open the history view widget
    void onShowAllHistory();

    /// Connects the signals emitted by the web view to slots handled by the browser window
    void onNewTabCreated(WebWidget *ww);

    /// Called when the user wants to view the source code of the current web page
    void onRequestViewSource();

    /// Called when the user requests that the contents of the current browser tab be printed
    void printTabContents();

    /// Called when a print preview dialog needs to generate a set of preview pages
    void onPrintPreviewRequested(QPrinter *printer, WebView *view);

    /// Callback handler when print operation is complete
    void onPrintFinished(bool success);

    /// Called when a link is hovered by the user
    void onLinkHovered(const QString &url);

    /// Called by the "Save Page As..." file menu option - spawns a save file dialog and passes the save event to the appropriate handler
    void onSavePageTriggered();

protected:
    /// Returns a pointer to the tab widget. Used by \ref SessionManager to save the browsing session
    BrowserTabWidget *getTabWidget() const;

    /// Called when the window is being closed
    void closeEvent(QCloseEvent *event) override;

    /// Handles drag enter events (supports tab drags)
    void dragEnterEvent(QDragEnterEvent *event) override;

    /// Handles tab drop events
    void dropEvent(QDropEvent *event) override;

    /// Handles resize events
    void resizeEvent(QResizeEvent *event) override;

private:
    /// UI items from .ui file
    Ui::MainWindow *ui;

    /// True if window is a private browsing window, false if else
    bool m_privateWindow;

    /// Pointer to browser settings
    Settings *m_settings;

    /// Service locator
    const ViperServiceLocator &m_serviceLocator;

    /// Bookmark manager
    BookmarkManager *m_bookmarkManager;

    /// Dialog used to clear recent history, cookies, etc.
    ClearHistoryDialog *m_clearHistoryDialog;

    /// Browser tab widget
    BrowserTabWidget *m_tabWidget;

    /// List of web action proxies that bind menu bar actions to web page actions
    QList<WebActionProxy*> m_webActions;

    /// Dialog that is shown when a bookmark is added or is being modified by the user
    BookmarkDialog *m_bookmarkDialog;

    /// Displays the link being hovered by the user in the status bar
    QLabel *m_linkHoverLabel;

    /// Flag indicating whether or not the window is being closed
    std::atomic_bool m_closing;

    /// Printer
    QPrinter m_printer;

    /// Printer event loop
    QEventLoop m_printLoop;

    /// Flag indicating if print preview is active or not
    bool m_isInPrintPreview;
};

#endif // MAINWINDOW_H
