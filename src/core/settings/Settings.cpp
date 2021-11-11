#include "Settings.h"

#include "HistoryManager.h"

#include <QDir>
#include <QFileInfo>
#include <QWebEngineSettings>
#include <QtWebEngineCoreVersion>

const QString Settings::Version = QStringLiteral("1.0");

Settings::Settings(QWebEngineSettings *webSettings) :
    QObject(nullptr),
    m_firstRun(false),
    m_settings(),
    m_storagePath(),
    m_settingMap {
        { BrowserSetting::StoragePath, QStringLiteral("StoragePath") },                { BrowserSetting::BookmarkPath, QStringLiteral("BookmarkPath") },
        { BrowserSetting::ExtensionStoragePath, QStringLiteral("ExtStoragePath") },    { BrowserSetting::HistoryPath, QStringLiteral("HistoryPath") },
        { BrowserSetting::FaviconPath, QStringLiteral("FaviconPath") },                { BrowserSetting::SearchEnginesFile, QStringLiteral("SearchEnginesFile") },
        { BrowserSetting::SessionFile, QStringLiteral("SessionFile") },                { BrowserSetting::UserAgentsFile, QStringLiteral("UserAgentsFile") },
        { BrowserSetting::UserScriptsConfig, QStringLiteral("UserScriptsConfig") },    { BrowserSetting::UserScriptsDir, QStringLiteral("UserScriptsDir") },
        { BrowserSetting::AdBlockPlusConfig, QStringLiteral("AdBlockPlusConfig") },    { BrowserSetting::ExemptThirdPartyCookieFile, QStringLiteral("ExemptThirdPartyCookieFile") },
        { BrowserSetting::AdBlockPlusDataDir, QStringLiteral("AdBlockPlusDataDir") },  { BrowserSetting::FixedFontSize, QStringLiteral("FixedFontSize") },
        { BrowserSetting::HomePage, QStringLiteral("HomePage") },                      { BrowserSetting::StartupMode, QStringLiteral("StartupMode") },
        { BrowserSetting::NewTabPage, QStringLiteral("NewTabPage") },                  { BrowserSetting::DownloadDir, QStringLiteral("DownloadDir") },
        { BrowserSetting::SendDoNotTrack, QStringLiteral("SendDoNotTrack") },          { BrowserSetting::AskWhereToSaveDownloads, QStringLiteral("AskWhereToSaveDownloads") },
        { BrowserSetting::EnableJavascript, QStringLiteral("EnableJavascript") },      { BrowserSetting::EnableJavascriptPopups, QStringLiteral("EnableJavascriptPopups") },
        { BrowserSetting::AutoLoadImages, QStringLiteral("AutoLoadImages") },          { BrowserSetting::EnablePlugins, QStringLiteral("EnablePlugins") },
        { BrowserSetting::EnableCookies, QStringLiteral("EnableCookies") },            { BrowserSetting::EnableThirdPartyCookies, QStringLiteral("EnableThirdPartyCookies") },
        { BrowserSetting::EnableXSSAudit, QStringLiteral("EnableXSSAudit") },          { BrowserSetting::CookiesDeleteWithSession, QStringLiteral("CookiesDeleteWithSession") },
        { BrowserSetting::EnableBookmarkBar, QStringLiteral("EnableBookmarkBar") },    { BrowserSetting::CustomUserAgent, QStringLiteral("CustomUserAgent") },
        { BrowserSetting::UserScriptsEnabled, QStringLiteral("UserScriptsEnabled") },  { BrowserSetting::AdBlockPlusEnabled, QStringLiteral("AdBlockPlusEnabled") },
        { BrowserSetting::InspectorPort, QStringLiteral("InspectorPort") },            { BrowserSetting::HistoryStoragePolicy, QStringLiteral("HistoryStoragePolicy") },
        { BrowserSetting::StandardFont, QStringLiteral("StandardFont") },              { BrowserSetting::ScrollAnimatorEnabled, QStringLiteral("ScrollAnimatorEnabled") },
        { BrowserSetting::SerifFont, QStringLiteral("SerifFont") },                    { BrowserSetting::OpenAllTabsInBackground, QStringLiteral("OpenAllTabsInBackground") },
        { BrowserSetting::SansSerifFont, QStringLiteral("SansSerifFont") },            { BrowserSetting::CursiveFont, QStringLiteral("CursiveFont") },
        { BrowserSetting::FantasyFont, QStringLiteral("FantasyFont") },                { BrowserSetting::FixedFont, QStringLiteral("FixedFont") },
        { BrowserSetting::StandardFontSize, QStringLiteral("StandardFontSize") },      { BrowserSetting::EnableAutoFill, QStringLiteral("EnableAutoFill") },
        { BrowserSetting::CachePath, QStringLiteral("CachePath") },                    { BrowserSetting::ThumbnailPath, QStringLiteral("ThumbnailPath") },
        { BrowserSetting::FavoritePagesFile, QStringLiteral("FavoritePagesFile") },    { BrowserSetting::Version, QStringLiteral("Version") }
    },
    m_webSettings(webSettings)
{
    setObjectName(QStringLiteral("Settings"));

    // Check if defaults need to be set
    if (!m_settings.contains(QStringLiteral("StoragePath")))
        setDefaults();

    // Check if settings needs to be updated
    if (m_settings.value(QStringLiteral("Version")).toString().compare(Version) != 0)
        updateSettings();

    m_storagePath = m_settings.value(QStringLiteral("StoragePath")).toString();
}

QString Settings::getPathValue(BrowserSetting key)
{
    return m_storagePath + m_settings.value(m_settingMap.value(key, QStringLiteral("unknown"))).toString();
}

QVariant Settings::getValue(BrowserSetting key)
{
    return m_settings.value(m_settingMap.value(key, QStringLiteral("unknown")));
}

void Settings::setValue(BrowserSetting key, const QVariant &value)
{
    if (getValue(key) == value)
        return;

    m_settings.setValue(m_settingMap.value(key, QStringLiteral("unknown")), value);

    emit settingChanged(key, value);
}

bool Settings::firstRun() const
{
    return m_firstRun;
}

void Settings::setDefaults()
{
    m_firstRun = true;

    QFileInfo settingsFileInfo(m_settings.fileName());

#ifdef Q_OS_WIN
    QDir settingsDir = QDir(QDir::homePath() + QString("\\AppData\\Local\\Vaccarelli"));
#else
    QDir settingsDir = settingsFileInfo.absoluteDir();
#endif
    QString settingsPath = settingsDir.absolutePath();
    if (!settingsDir.exists())
        settingsDir.mkpath(settingsPath);
    settingsPath.append(QDir::separator());

    QString cachePath =
            QString("%1%2%3%2%4").arg(QDir::homePath()).arg(QDir::separator()).arg(QStringLiteral(".cache")).arg(QStringLiteral("Vaccarelli"));

    m_settings.setValue(QStringLiteral("StoragePath"), settingsPath);
    m_settings.setValue(QStringLiteral("CachePath"), cachePath);
    m_settings.setValue(QStringLiteral("BookmarkPath"), QStringLiteral("bookmarks.db"));
    m_settings.setValue(QStringLiteral("CookiePath"), QStringLiteral("cookies.db"));
    m_settings.setValue(QStringLiteral("ExtStoragePath"), QStringLiteral("extension_storage.db"));
    m_settings.setValue(QStringLiteral("HistoryPath"), QStringLiteral("history.db"));
    m_settings.setValue(QStringLiteral("FaviconPath"), QStringLiteral("favicons.db"));
    m_settings.setValue(QStringLiteral("FavoritePagesFile"), QStringLiteral("favorite_pages.json"));
    m_settings.setValue(QStringLiteral("ThumbnailPath"), QStringLiteral("web_thumbnails.db"));
    m_settings.setValue(QStringLiteral("UserAgentsFile"),  QStringLiteral("user_agents.json"));
    m_settings.setValue(QStringLiteral("SearchEnginesFile"), QStringLiteral("search_engines.json"));
    m_settings.setValue(QStringLiteral("SessionFile"), QStringLiteral("last_session.json"));
    m_settings.setValue(QStringLiteral("UserScriptsDir"), QStringLiteral("UserScripts"));
    m_settings.setValue(QStringLiteral("UserScriptsConfig"), QStringLiteral("user_scripts.json"));
    m_settings.setValue(QStringLiteral("AdBlockPlusConfig"), QStringLiteral("adblock_plus.json"));
    m_settings.setValue(QStringLiteral("AdBlockPlusDataDir"), QStringLiteral("AdBlockPlus"));
    m_settings.setValue(QStringLiteral("ExemptThirdPartyCookieFile"), QStringLiteral("exempt_third_party_cookies.txt"));

    m_settings.setValue(QStringLiteral("HomePage"), QStringLiteral("https://www.startpage.com/"));
    m_settings.setValue(QStringLiteral("StartupMode"), static_cast<int>(StartupMode::LoadHomePage));
    m_settings.setValue(QStringLiteral("NewTabPage"), static_cast<int>(NewTabType::HomePage));
    m_settings.setValue(QStringLiteral("DownloadDir"), QDir::homePath() + QDir::separator() + "Downloads");
    m_settings.setValue(QStringLiteral("AskWhereToSaveDownloads"), false);
    m_settings.setValue(QStringLiteral("SendDoNotTrack"), false);
    m_settings.setValue(QStringLiteral("EnableJavascript"), true);
    m_settings.setValue(QStringLiteral("EnableJavascriptPopups"), false);
    m_settings.setValue(QStringLiteral("AutoLoadImages"), true);
    m_settings.setValue(QStringLiteral("EnablePlugins"), false);
    m_settings.setValue(QStringLiteral("EnableCookies"), true);
    m_settings.setValue(QStringLiteral("CookiesDeleteWithSession"), false);
    m_settings.setValue(QStringLiteral("EnableThirdPartyCookies"), true);
    m_settings.setValue(QStringLiteral("EnableXSSAudit"), true);
    m_settings.setValue(QStringLiteral("EnableBookmarkBar"), false);
    m_settings.setValue(QStringLiteral("CustomUserAgent"), false);
    m_settings.setValue(QStringLiteral("UserScriptsEnabled"), true);
    m_settings.setValue(QStringLiteral("AdBlockPlusEnabled"), true);
    m_settings.setValue(QStringLiteral("HistoryStoragePolicy"), static_cast<int>(HistoryStoragePolicy::Remember));
    m_settings.setValue(QStringLiteral("ScrollAnimatorEnabled"), false);
    m_settings.setValue(QStringLiteral("OpenAllTabsInBackground"), false);

    m_settings.setValue(QStringLiteral("StandardFont"), m_webSettings->fontFamily(QWebEngineSettings::StandardFont));
    m_settings.setValue(QStringLiteral("SerifFont"), m_webSettings->fontFamily(QWebEngineSettings::SerifFont));
    m_settings.setValue(QStringLiteral("SansSerifFont"), m_webSettings->fontFamily(QWebEngineSettings::SansSerifFont));
    m_settings.setValue(QStringLiteral("CursiveFont"), m_webSettings->fontFamily(QWebEngineSettings::CursiveFont));
    m_settings.setValue(QStringLiteral("FantasyFont"), m_webSettings->fontFamily(QWebEngineSettings::FantasyFont));
    m_settings.setValue(QStringLiteral("FixedFont"), m_webSettings->fontFamily(QWebEngineSettings::FixedFont));

    m_settings.setValue(QStringLiteral("StandardFontSize"), m_webSettings->fontSize(QWebEngineSettings::DefaultFontSize));
    m_settings.setValue(QStringLiteral("FixedFontSize"), m_webSettings->fontSize(QWebEngineSettings::DefaultFixedFontSize));

    m_settings.setValue(QStringLiteral("Version"), Version);
}

void Settings::updateSettings()
{
    const QString userVersion = m_settings.value(QStringLiteral("Version")).toString();
    if (userVersion.isEmpty())
    {
        // Cache path is only new setting added with the first versioning of settings
        QString cachePath =
                QString("%1%2%3%2%4").arg(QDir::homePath()).arg(QDir::separator()).arg(QStringLiteral(".cache")).arg(QStringLiteral("Vaccarelli"));
        m_settings.setValue(QStringLiteral("CachePath"), cachePath);
    }

    bool ok = false;
    float versionNumber = m_settings.value(QStringLiteral("Version")).toFloat(&ok);
    if (!ok || versionNumber < 0.9f)
        m_settings.setValue(QStringLiteral("ThumbnailPath"), QStringLiteral("web_thumbnails.db"));
    if (!ok || versionNumber < 1.0f)
    {
        m_settings.setValue(QStringLiteral("NewTabPage"), static_cast<int>(NewTabType::BlankPage));
        m_settings.setValue(QStringLiteral("FavoritePagesFile"), QStringLiteral("favorite_pages.json"));
    }

    m_settings.setValue(QStringLiteral("Version"), Version);
}
