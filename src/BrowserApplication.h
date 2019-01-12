#ifndef BROWSERAPPLICATION_H
#define BROWSERAPPLICATION_H

#include <memory>
#include <QApplication>
#include <QDateTime>
#include <QList>
#include <QPointer>

#include "BookmarkNode.h"
#include "ClearHistoryOptions.h"
#include "SessionManager.h"

class AutoFill;
class BlockedSchemeHandler;
class BookmarkManager;
class CookieJar;
class CookieWidget;
class DownloadManager;
class ExtStorage;
class FaviconStore;
class FavoritePagesManager;
class HistoryManager;
class MainWindow;
class RequestInterceptor;
class Settings;
class UserAgentManager;
class UserScriptManager;
class NetworkAccessManager;
class ViperSchemeHandler;

class QWebEngineProfile;

/// Potential modes of operation for the browser startup routine (ie load a home page, restore session, etc)
enum class StartupMode
{
    LoadHomePage   = 0,
    LoadBlankPage  = 1,
    RestoreSession = 2
};

Q_DECLARE_METATYPE(StartupMode)

/**
 * @class BrowserApplication
 * @brief Handles application functionality that is not confined to a single window
 */
class BrowserApplication : public QApplication
{
    Q_OBJECT

    friend class HistoryManager;
    friend class MainWindow;

public:
    /// BrowserApplication constructor
    BrowserApplication(int &argc, char **argv);

    /// BrowserApplication destructor
    ~BrowserApplication();

    /// Returns the browser application singleton
    static BrowserApplication *instance();

    /// Returns a pointer to the AutoFill manager
    AutoFill *getAutoFill();

    /// Returns a pointer to the bookmark manager
    BookmarkManager *getBookmarkManager();

    /// Returns the cookie jar used for standard (non-private) browsing mode
    CookieJar *getCookieJar();

    /// Returns a pointer to the application settings object
    Settings *getSettings();

    /// Returns the download manager
    DownloadManager *getDownloadManager();

    /// Returns the favicon storage manager
    FaviconStore *getFaviconStore();

    /// Returns a pointer to the object that maintains a list of the user's favorite web pages
    FavoritePagesManager *getFavoritePagesManager();

    /// Returns the history manager
    HistoryManager *getHistoryManager();

    /// Returns the network access manager
    NetworkAccessManager *getNetworkAccessManager();

    /// Returns a pointer to the private web browsing profile
    QWebEngineProfile *getPrivateBrowsingProfile();

    /// Returns a pointer to the user agent manager
    UserAgentManager *getUserAgentManager();

    /// Returns a pointer to the user script manager
    UserScriptManager *getUserScriptManager();

    /// Returns the cookie manager
    CookieWidget *getCookieManager();

    /// Returns a pointer to the extension storage object
    ExtStorage *getExtStorage();

signals:
    /// Emitted when each browsing window's history menu should reset its contents
    void resetHistoryMenu();

public slots:
    /// Spawns and returns the pointer to a new browser window
    MainWindow *getNewWindow();

    /// Spawns a new browser window, setting it to private browsing mode before returning a pointer to the window
    MainWindow *getNewPrivateWindow();

private slots:
    /// Called when the aboutToQuit signal is emitted. If the user has enabled the session restore feature, their
    /// current windows and tabs will be saved so they can be opened at the start of the next browsing session
    void beforeBrowserQuit();

    /// Called when a browser window is about to be closed. If it is the last window still open, is not private,
    /// and the user enabled the session save feature, this will save the browsing session
    void maybeSaveSession();

protected:
    /// Clears the given history type(s) from the browser's storage, beginning with the start time until the present.
    /// If no start time is given, all history will be cleared
    void clearHistory(HistoryType histType, QDateTime start = QDateTime());

    /// Clears the given history type(s) from the browser's storage within the given {start, end} date-time range
    void clearHistoryRange(HistoryType histType, std::pair<QDateTime, QDateTime> range);

private:
    /// Installs core browser scripts into the script collection
    void installGlobalWebScripts();

    /// Initializes the standard and private web profiles used by the browser.
    /// This includes instantiation of request interceptors and custom scheme handlers.
    void setupWebProfiles();

private:
    /// Application settings
    std::unique_ptr<Settings> m_settings;

    /// Bookmark management class
    std::unique_ptr<BookmarkManager> m_bookmarks;

    /// AutoFill manager
    AutoFill *m_autoFill;

    /// Cookie jar
    CookieJar *m_cookieJar;

    /// Download manager
    DownloadManager *m_downloadMgr;

    /// Favicon storage manager
    std::unique_ptr<FaviconStore> m_faviconStorage;

    /// Web history manager
    std::unique_ptr<HistoryManager> m_historyMgr;

    /// Network access manager
    NetworkAccessManager *m_networkAccessMgr;

    /// User agent manager
    UserAgentManager *m_userAgentMgr;

    /// User script manager
    UserScriptManager *m_userScriptMgr;

    /// List of browser windows
    QList< QPointer<MainWindow> > m_browserWindows;

    /// Browsing session manager
    SessionManager m_sessionMgr;

    /// Request interceptor
    RequestInterceptor *m_requestInterceptor;

    /// viper scheme handler
    ViperSchemeHandler *m_viperSchemeHandler;

    /// AdBlock redirect scheme handler
    BlockedSchemeHandler *m_blockedSchemeHandler;

    /// Private browsing profile
    QWebEngineProfile *m_privateProfile;

    /// Cookie manager
    CookieWidget *m_cookieUI;

    /// Web extension storage - used to store user script data on a per-script basis rather than per-site
    std::unique_ptr<ExtStorage> m_extStorage;

    /// Maintains a list of the user's favorite web pages
    FavoritePagesManager *m_favoritePagesMgr;
};

#define sBrowserApplication BrowserApplication::instance()

#endif // BROWSERAPPLICATION_H
