#ifndef BROWSERAPPLICATION_H
#define BROWSERAPPLICATION_H

#include <memory>
#include <QApplication>
#include <QDateTime>
#include <QList>
#include <QPointer>

#include "BookmarkNode.h"
#include "ClearHistoryOptions.h"
#include "DatabaseTaskScheduler.h"
#include "ServiceLocator.h"
#include "SessionManager.h"
#include "Settings.h"

namespace adblock {
    class AdBlockManager;
}

class AutoFill;
class BlockedSchemeHandler;
class BookmarkManager;
class BrowserIPC;
class CookieJar;
class CookieWidget;
class DownloadManager;
class ExtStorage;
class FaviconManager;
class FavoritePagesManager;
class HistoryManager;
class MainWindow;
class NetworkAccessManager;
class RequestInterceptor;
class Settings;
class UserAgentManager;
class UserScriptManager;
class ViperSchemeHandler;
class WebPageThumbnailStore;
class WebSettings;

class QWebEngineProfile;

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
    explicit BrowserApplication(BrowserIPC *ipc, int &argc, char **argv);

    /// BrowserApplication destructor
    ~BrowserApplication();

    /// Returns the browser application singleton
    static BrowserApplication *instance();

    /// Returns the advertisement blocking manager
    adblock::AdBlockManager *getAdBlockManager();

    /// Returns a pointer to the AutoFill manager
    AutoFill *getAutoFill();

    /// Returns a pointer to the application settings object
    Settings *getSettings();

    /// Returns the download manager
    DownloadManager *getDownloadManager();

    /// Returns a pointer to the favicon manager
    FaviconManager *getFaviconManager();

    /// Returns the network access manager
    NetworkAccessManager *getNetworkAccessManager();

    /// Returns a pointer to the private web browsing profile
    QWebEngineProfile *getPrivateBrowsingProfile();

    /// Searches for a window with the given identifier, returning a pointer to the
    /// MainWindow if found, or a nullptr otherwise.
    MainWindow *getWindowById(WId windowId) const;

    /// Wrapper around service locator call
    QObject *getService(const QString &serviceName) const;

    /// Returns true if the application is *likely* using a dark theme
    bool isDarkTheme() const;

Q_SIGNALS:
    /// Emitted when any and all runtime plugins have been loaded into the application
    void pluginsLoaded();

public Q_SLOTS:
    /// Graceful exit handler
    void prepareToQuit();

    /// Spawns and returns the pointer to a new browser window
    MainWindow *getNewWindow();

    /// Spawns a new browser window, setting it to private browsing mode before returning a pointer to the window
    MainWindow *getNewPrivateWindow();

    /// Opens the given url in the active browser window
    void openUrl(const QUrl &url);

private Q_SLOTS:
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

protected:
    /// Called on a regular interval to check for messages in the IPC channel
    void timerEvent(QTimerEvent *event) override;

private:
    /// Installs core browser scripts into the script collection
    void installGlobalWebScripts();

    /// Registers a service with the \ref ViperServiceLocator
    void registerService(QObject *service);

    /// Initializes the standard and private web profiles used by the browser.
    /// This includes instantiation of request interceptors and custom scheme handlers.
    void setupWebProfiles();

    /// Checks for any unread inter-process messages. This would be empty, or it would
    /// contain a request to open one or more URLs.
    void checkBrowserIPC();

    /// Loads any dynamic plugins found in the installation directory
    void loadPlugins();

private:
    /// Inter-process communication handler
    BrowserIPC *m_ipc;

    /// Unique identifier of the timer that is used to check for pending messages in the IPC channel
    int m_ipcTimerId;

    /// Application settings
    Settings *m_settings;

    /// Web-related settings manager
    WebSettings *m_webSettings;

    /// Bookmark manager
    BookmarkManager *m_bookmarkManager;

    /// Advertisement blocking system manager
    adblock::AdBlockManager *m_adBlockManager;

    /// AutoFill manager
    AutoFill *m_autoFill;

    /// Cookie jar
    CookieJar *m_cookieJar;

    /// Download manager
    DownloadManager *m_downloadMgr;

    /// Favicon manager
    FaviconManager *m_faviconMgr;

    /// Web history manager
    HistoryManager *m_historyMgr;

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

    /// Ad-block request redirection scheme handler
    BlockedSchemeHandler *m_blockedSchemeHandler;

    /// Regular browsing profile
    QWebEngineProfile *m_defaultProfile;

    /// Private browsing profile
    QWebEngineProfile *m_privateProfile;

    /// Cookie manager
    CookieWidget *m_cookieUI;

    /// Web extension storage - used to store user script data on a per-script basis rather than per-site
    std::unique_ptr<ExtStorage> m_extStorage;

    /// Maintains a list of the user's favorite web pages
    FavoritePagesManager *m_favoritePagesMgr;

    /// Web page thumbnail storage manager
    std::unique_ptr<WebPageThumbnailStore> m_thumbnailStore;

    /// Service locator - stores the bookmark manager, history manager, favicon manager, and other important services
    ViperServiceLocator m_serviceLocator;

    /// Database worker task scheduler
    DatabaseTaskScheduler m_databaseScheduler;
};

#define sBrowserApplication BrowserApplication::instance()

#endif // BROWSERAPPLICATION_H
