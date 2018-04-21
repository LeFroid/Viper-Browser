#include "Settings.h"

#include "BrowserApplication.h"
#include "UserAgentManager.h"
#include "WebPage.h"
#include <QDir>
#include <QFileInfo>
#include <QWebEngineProfile>
#include <QWebEngineSettings>

Settings::Settings() :
    m_firstRun(false),
    m_settings(),
    m_storagePath()
{
    // Check if defaults need to be set
    if (!m_settings.contains(QStringLiteral("StoragePath")))
        setDefaults();

    m_storagePath = m_settings.value(QStringLiteral("StoragePath")).toString();
}

QString Settings::getPathValue(const QString &key)
{
    return m_storagePath + m_settings.value(key).toString();
}

QVariant Settings::getValue(const QString &key)
{
    return m_settings.value(key);
}

void Settings::setValue(const QString &key, const QVariant &value)
{
    m_settings.setValue(key, value);
}

bool Settings::firstRun() const
{
    return m_firstRun;
}

void Settings::applyWebSettings()
{
    QWebEngineSettings *settings = QWebEngineSettings::defaultSettings();
    settings->setAttribute(QWebEngineSettings::JavascriptEnabled,        m_settings.value(QStringLiteral("EnableJavascript")).toBool());
    settings->setAttribute(QWebEngineSettings::JavascriptCanOpenWindows, m_settings.value(QStringLiteral("EnableJavascriptPopups")).toBool());
    settings->setAttribute(QWebEngineSettings::AutoLoadImages,           m_settings.value(QStringLiteral("AutoLoadImages")).toBool());
    settings->setAttribute(QWebEngineSettings::PluginsEnabled,           m_settings.value(QStringLiteral("EnablePlugins")).toBool());
    settings->setAttribute(QWebEngineSettings::XSSAuditingEnabled,       m_settings.value(QStringLiteral("EnableXSSAudit")).toBool());

    settings->setAttribute(QWebEngineSettings::LocalStorageEnabled,               true);

    settings->setFontFamily(QWebEngineSettings::StandardFont,       m_settings.value(QStringLiteral("StandardFont")).toString());
    settings->setFontFamily(QWebEngineSettings::SerifFont,          m_settings.value(QStringLiteral("SerifFont")).toString());
    settings->setFontFamily(QWebEngineSettings::SansSerifFont,      m_settings.value(QStringLiteral("SansSerifFont")).toString());
    settings->setFontFamily(QWebEngineSettings::CursiveFont,        m_settings.value(QStringLiteral("CursiveFont")).toString());
    settings->setFontFamily(QWebEngineSettings::FantasyFont,        m_settings.value(QStringLiteral("FantasyFont")).toString());
    settings->setFontFamily(QWebEngineSettings::FixedFont,          m_settings.value(QStringLiteral("FixedFont")).toString());

    settings->setFontSize(QWebEngineSettings::DefaultFontSize,      m_settings.value(QStringLiteral("StandardFontSize")).toInt());
    settings->setFontSize(QWebEngineSettings::DefaultFixedFontSize, m_settings.value(QStringLiteral("FixedFontSize")).toInt());

    QDir cachePath(QDir::homePath() + QDir::separator() + QStringLiteral(".cache") + QDir::separator() + QStringLiteral("Vaccarelli"));
    if (!cachePath.exists())
        cachePath.mkpath(cachePath.absolutePath());
    
    auto defaultProfile = QWebEngineProfile::defaultProfile();
    defaultProfile->setCachePath(cachePath.absolutePath());
    defaultProfile->setPersistentStoragePath(cachePath.absolutePath());

    // Check if custom user agent is used
    if (getValue(QStringLiteral("CustomUserAgent")).toBool())
        defaultProfile->setHttpUserAgent(sBrowserApplication->getUserAgentManager()->getUserAgent().Value);
}

void Settings::setDefaults()
{
    m_firstRun = true;

    QFileInfo settingsFileInfo(m_settings.fileName());

    QDir settingsDir = settingsFileInfo.absoluteDir();
    QString settingsPath = settingsDir.absolutePath();
    if (!settingsDir.exists())
        settingsDir.mkpath(settingsPath);
    settingsPath.append(QDir::separator());

    m_settings.setValue(QStringLiteral("StoragePath"), settingsPath);
    m_settings.setValue(QStringLiteral("BookmarkPath"), QStringLiteral("bookmarks.db"));
    m_settings.setValue(QStringLiteral("CookiePath"), QStringLiteral("cookies.db"));
    m_settings.setValue(QStringLiteral("HistoryPath"), QStringLiteral("history.db"));
    m_settings.setValue(QStringLiteral("FaviconPath"), QStringLiteral("favicons.db"));
    m_settings.setValue(QStringLiteral("UserAgentsFile"),  QStringLiteral("user_agents.json"));
    m_settings.setValue(QStringLiteral("SearchEnginesFile"), QStringLiteral("search_engines.json"));
    m_settings.setValue(QStringLiteral("SessionFile"), QStringLiteral("last_session.json"));
    m_settings.setValue(QStringLiteral("UserScriptsDir"), QStringLiteral("UserScripts"));
    m_settings.setValue(QStringLiteral("UserScriptsConfig"), QStringLiteral("user_scripts.json"));
    m_settings.setValue(QStringLiteral("AdBlockPlusConfig"), QStringLiteral("adblock_plus.json"));
    m_settings.setValue(QStringLiteral("AdBlockPlusDataDir"), QStringLiteral("AdBlockPlus"));

    m_settings.setValue(QStringLiteral("HomePage"), QStringLiteral("https://www.ixquick.com/"));
    m_settings.setValue(QStringLiteral("StartupMode"), QVariant::fromValue(StartupMode::LoadHomePage));
    m_settings.setValue(QStringLiteral("NewTabsLoadHomePage"), true);
    m_settings.setValue(QStringLiteral("DownloadDir"), QDir::homePath() + QDir::separator() + "Downloads");
    m_settings.setValue(QStringLiteral("AskWhereToSaveDownloads"), false);
    m_settings.setValue(QStringLiteral("EnableJavascript"), true);
    m_settings.setValue(QStringLiteral("EnableJavascriptPopups"), false);
    m_settings.setValue(QStringLiteral("AutoLoadImages"), true);
    m_settings.setValue(QStringLiteral("EnablePlugins"), false);
    m_settings.setValue(QStringLiteral("EnableCookies"), true);
    m_settings.setValue(QStringLiteral("EnableXSSAudit"), true);
    m_settings.setValue(QStringLiteral("EnableBookmarkBar"), false);
    m_settings.setValue(QStringLiteral("CustomUserAgent"), false);
    m_settings.setValue(QStringLiteral("UserScriptsEnabled"), true);
    m_settings.setValue(QStringLiteral("AdBlockPlusEnabled"), true);

    QWebEngineSettings *webSettings = QWebEngineSettings::defaultSettings();
    m_settings.setValue(QStringLiteral("StandardFont"), webSettings->fontFamily(QWebEngineSettings::StandardFont));
    m_settings.setValue(QStringLiteral("SerifFont"), webSettings->fontFamily(QWebEngineSettings::SerifFont));
    m_settings.setValue(QStringLiteral("SansSerifFont"), webSettings->fontFamily(QWebEngineSettings::SansSerifFont));
    m_settings.setValue(QStringLiteral("CursiveFont"), webSettings->fontFamily(QWebEngineSettings::CursiveFont));
    m_settings.setValue(QStringLiteral("FantasyFont"), webSettings->fontFamily(QWebEngineSettings::FantasyFont));
    m_settings.setValue(QStringLiteral("FixedFont"), webSettings->fontFamily(QWebEngineSettings::FixedFont));

    m_settings.setValue(QStringLiteral("StandardFontSize"), webSettings->fontSize(QWebEngineSettings::DefaultFontSize));
    m_settings.setValue(QStringLiteral("FixedFontSize"), webSettings->fontSize(QWebEngineSettings::DefaultFixedFontSize));

    // Todo: Store settings such as whether or not to store history
}
