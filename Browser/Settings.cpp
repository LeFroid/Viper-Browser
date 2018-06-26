#include "Settings.h"

#include "BrowserApplication.h"
#include "CookieJar.h"
#include "HistoryManager.h"
#include "UserAgentManager.h"
#include "WebPage.h"

#include <QDir>
#include <QFileInfo>
#include <QWebEngineProfile>
#include <QWebEngineSettings>
#include <QtWebEngineCoreVersion>

Settings::Settings() :
    m_firstRun(false),
    m_settings(),
    m_storagePath()
{
    // Check if defaults need to be set
    if (!m_settings.contains(QLatin1String("StoragePath")))
        setDefaults();

    m_storagePath = m_settings.value(QLatin1String("StoragePath")).toString();
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
    settings->setAttribute(QWebEngineSettings::FullScreenSupportEnabled, true);
    settings->setAttribute(QWebEngineSettings::JavascriptEnabled,        m_settings.value(QLatin1String("EnableJavascript"), true).toBool());
    settings->setAttribute(QWebEngineSettings::JavascriptCanOpenWindows, m_settings.value(QLatin1String("EnableJavascriptPopups"), false).toBool());
    settings->setAttribute(QWebEngineSettings::AutoLoadImages,           m_settings.value(QLatin1String("AutoLoadImages"), true).toBool());
    settings->setAttribute(QWebEngineSettings::PluginsEnabled,           m_settings.value(QLatin1String("EnablePlugins"), false).toBool());
    settings->setAttribute(QWebEngineSettings::XSSAuditingEnabled,       m_settings.value(QLatin1String("EnableXSSAudit"), true).toBool());
    settings->setAttribute(QWebEngineSettings::ScrollAnimatorEnabled,    m_settings.value(QLatin1String("ScrollAnimatorEnabled"), false).toBool());

    settings->setAttribute(QWebEngineSettings::LocalStorageEnabled,      true);

    settings->setFontFamily(QWebEngineSettings::StandardFont,       m_settings.value(QLatin1String("StandardFont")).toString());
    settings->setFontFamily(QWebEngineSettings::SerifFont,          m_settings.value(QLatin1String("SerifFont")).toString());
    settings->setFontFamily(QWebEngineSettings::SansSerifFont,      m_settings.value(QLatin1String("SansSerifFont")).toString());
    settings->setFontFamily(QWebEngineSettings::CursiveFont,        m_settings.value(QLatin1String("CursiveFont")).toString());
    settings->setFontFamily(QWebEngineSettings::FantasyFont,        m_settings.value(QLatin1String("FantasyFont")).toString());
    settings->setFontFamily(QWebEngineSettings::FixedFont,          m_settings.value(QLatin1String("FixedFont")).toString());

    settings->setFontSize(QWebEngineSettings::DefaultFontSize,      m_settings.value(QLatin1String("StandardFontSize")).toInt());
    settings->setFontSize(QWebEngineSettings::DefaultFixedFontSize, m_settings.value(QLatin1String("FixedFontSize")).toInt());

//#if (QTWEBENGINECORE_VERSION >= QT_VERSION_CHECK(5, 11, 0))
    // can cause issues on some websites, will keep disabled until a fix is found
    //settings->setAttribute(QWebEngineSettings::PlaybackRequiresUserGesture, true);
//#endif

    HistoryStoragePolicy historyPolicy = static_cast<HistoryStoragePolicy>(m_settings.value(QLatin1String("HistoryStoragePolicy")).toInt());
    sBrowserApplication->getHistoryManager()->setStoragePolicy(historyPolicy);

    CookieJar *cookieJar = sBrowserApplication->getCookieJar();
    cookieJar->setCookiesEnabled(m_settings.value(QLatin1String("EnableCookies")).toBool());
    cookieJar->setThirdPartyCookiesEnabled(m_settings.value(QLatin1String("EnableThirdPartyCookies")).toBool());

    QWebEngineProfile::PersistentCookiesPolicy cookiesPolicy = QWebEngineProfile::AllowPersistentCookies;
    if (m_settings.value(QLatin1String("CookiesDeleteWithSession")).toBool())
        cookiesPolicy = QWebEngineProfile::NoPersistentCookies;

    auto defaultProfile = QWebEngineProfile::defaultProfile();
    defaultProfile->setPersistentCookiesPolicy(cookiesPolicy);

    QDir cachePath(QDir::homePath() + QDir::separator() + QStringLiteral(".cache") + QDir::separator() + QStringLiteral("Vaccarelli"));
    if (!cachePath.exists())
        cachePath.mkpath(cachePath.absolutePath());
    
    defaultProfile->setCachePath(cachePath.absolutePath());
    defaultProfile->setPersistentStoragePath(cachePath.absolutePath());

    // Check if custom user agent is used
    if (getValue(QLatin1String("CustomUserAgent")).toBool())
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

    m_settings.setValue(QLatin1String("StoragePath"), settingsPath);
    m_settings.setValue(QLatin1String("BookmarkPath"), QLatin1String("bookmarks.db"));
    m_settings.setValue(QLatin1String("CookiePath"), QLatin1String("cookies.db"));
    m_settings.setValue(QLatin1String("ExtStoragePath"), QLatin1String("extension_storage.db"));
    m_settings.setValue(QLatin1String("HistoryPath"), QLatin1String("history.db"));
    m_settings.setValue(QLatin1String("FaviconPath"), QLatin1String("favicons.db"));
    m_settings.setValue(QLatin1String("UserAgentsFile"),  QLatin1String("user_agents.json"));
    m_settings.setValue(QLatin1String("SearchEnginesFile"), QLatin1String("search_engines.json"));
    m_settings.setValue(QLatin1String("SessionFile"), QLatin1String("last_session.json"));
    m_settings.setValue(QLatin1String("UserScriptsDir"), QLatin1String("UserScripts"));
    m_settings.setValue(QLatin1String("UserScriptsConfig"), QLatin1String("user_scripts.json"));
    m_settings.setValue(QLatin1String("AdBlockPlusConfig"), QLatin1String("adblock_plus.json"));
    m_settings.setValue(QLatin1String("AdBlockPlusDataDir"), QLatin1String("AdBlockPlus"));
    m_settings.setValue(QLatin1String("ExemptThirdPartyCookieFile"), QLatin1String("exempt_third_party_cookies.txt"));

    m_settings.setValue(QLatin1String("HomePage"), QLatin1String("https://www.startpage.com/"));
    m_settings.setValue(QLatin1String("StartupMode"), static_cast<int>(StartupMode::LoadHomePage));
    m_settings.setValue(QLatin1String("NewTabsLoadHomePage"), true);
    m_settings.setValue(QLatin1String("DownloadDir"), QDir::homePath() + QDir::separator() + "Downloads");
    m_settings.setValue(QLatin1String("AskWhereToSaveDownloads"), false);
    m_settings.setValue(QLatin1String("EnableJavascript"), true);
    m_settings.setValue(QLatin1String("EnableJavascriptPopups"), false);
    m_settings.setValue(QLatin1String("AutoLoadImages"), true);
    m_settings.setValue(QLatin1String("EnablePlugins"), false);
    m_settings.setValue(QLatin1String("EnableCookies"), true);
    m_settings.setValue(QLatin1String("CookiesDeleteWithSession"), false);
    m_settings.setValue(QLatin1String("EnableThirdPartyCookies"), true);
    m_settings.setValue(QLatin1String("EnableXSSAudit"), true);
    m_settings.setValue(QLatin1String("EnableBookmarkBar"), false);
    m_settings.setValue(QLatin1String("CustomUserAgent"), false);
    m_settings.setValue(QLatin1String("UserScriptsEnabled"), true);
    m_settings.setValue(QLatin1String("AdBlockPlusEnabled"), true);
#if (QTWEBENGINECORE_VERSION < QT_VERSION_CHECK(5, 11, 0))
    m_settings.setValue(QLatin1String("InspectorPort"), 9477);
#endif
    m_settings.setValue(QLatin1String("HistoryStoragePolicy"), static_cast<int>(HistoryStoragePolicy::Remember));
    m_settings.setValue(QLatin1String("ScrollAnimatorEnabled"), false);
    m_settings.setValue(QLatin1String("OpenAllTabsInBackground"), false);

    QWebEngineSettings *webSettings = QWebEngineSettings::defaultSettings();
    m_settings.setValue(QLatin1String("StandardFont"), webSettings->fontFamily(QWebEngineSettings::StandardFont));
    m_settings.setValue(QLatin1String("SerifFont"), webSettings->fontFamily(QWebEngineSettings::SerifFont));
    m_settings.setValue(QLatin1String("SansSerifFont"), webSettings->fontFamily(QWebEngineSettings::SansSerifFont));
    m_settings.setValue(QLatin1String("CursiveFont"), webSettings->fontFamily(QWebEngineSettings::CursiveFont));
    m_settings.setValue(QLatin1String("FantasyFont"), webSettings->fontFamily(QWebEngineSettings::FantasyFont));
    m_settings.setValue(QLatin1String("FixedFont"), webSettings->fontFamily(QWebEngineSettings::FixedFont));

    m_settings.setValue(QLatin1String("StandardFontSize"), webSettings->fontSize(QWebEngineSettings::DefaultFontSize));
    m_settings.setValue(QLatin1String("FixedFontSize"), webSettings->fontSize(QWebEngineSettings::DefaultFixedFontSize));
}
