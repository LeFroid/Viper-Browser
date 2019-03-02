#ifndef BROWSERSETTING_H
#define BROWSERSETTING_H

/// Potential modes of operation for the browser startup routine (ie load a home page, restore session, etc)
enum class StartupMode
{
    LoadHomePage   = 0,
    LoadBlankPage  = 1,
    LoadNewTabPage = 2,
    RestoreSession = 3
};

/// List of configurable browser settings
enum class BrowserSetting
{
    /// Base path of configuration files and data stores
    StoragePath,

    /// Path to the browser's cache
    CachePath,

    /// Path to the bookmarks database file, relative to the storage path
    BookmarkPath,

    /// Path to the web extensions database file, relative to the storage path
    ExtensionStoragePath,

    /// Path to the browsing history database file, relative to the storage path
    HistoryPath,

    /// Path to the favicon database file, relative to the storage path
    FaviconPath,

    /// Path to the data file containing pinned pages to be shown in the "New Tab" web page
    FavoritePagesFile,

    /// Path to the web page thumbnail database file, relative to the storage path
    ThumbnailPath,

    /// Name of the search engine information file
    SearchEnginesFile,

    /// Name of the last browsing session information file
    SessionFile,

    /// Name of the user agent collection file
    UserAgentsFile,

    /// Name of the user scripts configuration file
    UserScriptsConfig,

    /// Path of the user scripts storage directory, relative to the storage path
    UserScriptsDir,

    /// Name of the ad blocking configuration file
    AdBlockPlusConfig,

    /// Path of the ad block filter and resource storage directory, relative to the storage path
    AdBlockPlusDataDir,

    /// Name of the third party cookie setting exception file
    ExemptThirdPartyCookieFile,

    /// Home page URL
    HomePage,

    /// Determines the behavior of the browser when it is started
    StartupMode,

    /// Stores the type of page that is loaded by default for every new web page (see \ref NewTabType )
    NewTabPage,

    /// User download directory
    DownloadDir,

    /// Determines if the download path should be confirmed before any files are downloaded
    AskWhereToSaveDownloads,

    /// Determines whether or not the 'Do Not Track' header should be sent with network requests
    SendDoNotTrack,

    /// Determines whether or not the form AutoFill system is enabled
    EnableAutoFill,

    /// Determines whether or not JavaScript should be enabled
    EnableJavascript,

    /// Determines whether or not JavaScript can open popups
    EnableJavascriptPopups,

    /// Determines whether or not images should be loaded automatically
    AutoLoadImages,

    /// Determines whether or not browser plugins, such as flash, should be enabled
    EnablePlugins,

    /// Determines whether or not cookies can be stored
    EnableCookies,

    /// Determines whether or not third party domains can store cookies when browsing a website
    EnableThirdPartyCookies,

    /// Determines whether or not cookies can be stored after the browser application is closed
    CookiesDeleteWithSession,

    /// Determines whether load requests should be monitored for cross-site scripting attempts
    EnableXSSAudit,

    /// Determines whether or not the bookmark bar should be shown by default
    EnableBookmarkBar,

    /// Determines whether or not a custom user agent string should be sent with network requests
    CustomUserAgent,

    /// Determines whether or not the user script system is enabled
    UserScriptsEnabled,

    /// Determines whether or not the advertisement and malicious content blocking system is enabled
    AdBlockPlusEnabled,

    /// Port of the remote web inspector (for QtWebEngine versions < 5.11)
    InspectorPort,

    /// History storage policy - see \ref HistoryStoragePolicy
    HistoryStoragePolicy,

    /// Determines whether the scroll animator should be enabled
    ScrollAnimatorEnabled,

    /// Determines whether or not all new tabs should be opened in the background, without switching from the current tab
    OpenAllTabsInBackground,

    /// Standard font
    StandardFont,

    /// Serif font
    SerifFont,

    /// Sans-serif font
    SansSerifFont,

    /// Cursive font
    CursiveFont,

    /// Fantasy font
    FantasyFont,

    /// Fixed font
    FixedFont,

    /// Standard font family size
    StandardFontSize,

    /// Fixed font size
    FixedFontSize,

    /// Settings file version
    Version
};

#endif // BROWSERSETTING_H
