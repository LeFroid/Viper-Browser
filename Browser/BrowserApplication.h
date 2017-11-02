#ifndef BROWSERAPPLICATION_H
#define BROWSERAPPLICATION_H

#include <memory>
#include <QApplication>
#include <QDateTime>
#include <QList>
#include <QPointer>

#include "History/ClearHistoryOptions.h"

class BookmarkManager;
class CookieJar;
class DownloadManager;
class FaviconStorage;
class HistoryManager;
class HistoryWidget;
class MainWindow;
class Settings;
class URLSuggestionModel;
class UserAgentManager;
class NetworkAccessManager;

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

    friend class MainWindow;

public:
    /// BrowserApplication constructor
    BrowserApplication(int &argc, char **argv);

    /// BrowserApplication destructor
    ~BrowserApplication();

    /// Returns the browser application singleton
    static BrowserApplication *instance();

    /// Returns a shared pointer to the bookmark manager
    std::shared_ptr<BookmarkManager> getBookmarkManager();

    /// Returns the cookie jar used when in non-private browsing modes
    std::shared_ptr<CookieJar> getCookieJar();

    /// Returns a shared pointer to the application settings object
    std::shared_ptr<Settings> getSettings();

    /// Returns the download manager
    DownloadManager *getDownloadManager();

    /// Returns the favicon storage manager
    FaviconStorage *getFaviconStorage();

    /// Returns the history manager
    HistoryManager *getHistoryManager();

    /// Returns the history widget
    HistoryWidget *getHistoryWidget();

    /// Returns the network access manager
    NetworkAccessManager *getNetworkAccessManager();

    /// Returns a network access manager for private browsing
    NetworkAccessManager *getPrivateNetworkAccessManager();

    /// Returns a pointer to the model used for suggesting URLs for the user to visit
    URLSuggestionModel *getURLSuggestionModel();

    /// Returns a pointer to the user agent manager
    UserAgentManager *getUserAgentManager();

public slots:
    /// Spawns and returns the pointer to a new browser window
    MainWindow *getNewWindow();

    /// Spawns a new browser window, setting it to private browsing mode before returning a pointer to the window
    MainWindow *getNewPrivateWindow();

    /// Updates all browser windows' bookmark menus / toolbars
    void updateBookmarkMenus();

protected:
    /// Clears the given history type(s) from the browser's storage, beginning with the start time until the present.
    /// If no start time is given, all history will be cleared
    void clearHistory(HistoryType histType, QDateTime start = QDateTime());

private:
    /// Resets each browser's history menu after clearing recent history
    void resetHistoryMenus();

    /// Resets each browser's user agent menu after a new agent has been added
    void resetUserAgentMenus();

    /// Sets the global web settings of the application - called during initialization
    void setWebSettings();

private:
    /// Application settings
    std::shared_ptr<Settings> m_settings;

    /// Bookmark management class
    std::shared_ptr<BookmarkManager> m_bookmarks;

    /// Cookie jar
    std::shared_ptr<CookieJar> m_cookieJar;

    /// Download manager
    DownloadManager *m_downloadMgr;

    /// Favicon storage manager
    FaviconStorage *m_faviconStorage;

    /// History management class
    HistoryManager *m_historyMgr;

    /// History view UI
    HistoryWidget *m_historyWidget;

    /// URL suggestion model for browser windows
    URLSuggestionModel *m_suggestionModel;

    /// Network access manager
    NetworkAccessManager *m_networkAccessMgr;

    /// Private browsing network access manager
    NetworkAccessManager *m_privateNetworkAccessMgr;

    /// User agent manager
    UserAgentManager *m_userAgentMgr;

    /// List of browser windows
    QList< QPointer<MainWindow> > m_browserWindows;
};

#define sBrowserApplication BrowserApplication::instance()

#endif // BROWSERAPPLICATION_H
