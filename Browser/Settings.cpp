#include "Settings.h"

#include "BrowserApplication.h"
#include "UserAgentManager.h"
#include "WebPage.h"
#include <QDir>
#include <QFileInfo>
#include <QWebSettings>

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
    // Check if custom user agent is used
    if (getValue(QStringLiteral("CustomUserAgent")).toBool())
        WebPage::setUserAgent(sBrowserApplication->getUserAgentManager()->getUserAgent().Value);

    QWebSettings *settings = QWebSettings::globalSettings();
    settings->setAttribute(QWebSettings::JavascriptEnabled,        m_settings.value(QStringLiteral("EnableJavascript")).toBool());
    settings->setAttribute(QWebSettings::JavascriptCanOpenWindows, m_settings.value(QStringLiteral("EnableJavascriptPopups")).toBool());
    settings->setAttribute(QWebSettings::AutoLoadImages,           m_settings.value(QStringLiteral("AutoLoadImages")).toBool());
    settings->setAttribute(QWebSettings::PluginsEnabled,           m_settings.value(QStringLiteral("EnablePlugins")).toBool());
    settings->setAttribute(QWebSettings::XSSAuditingEnabled,       m_settings.value(QStringLiteral("EnableXSSAudit")).toBool());

    settings->setAttribute(QWebSettings::DeveloperExtrasEnabled,            true);
    settings->setAttribute(QWebSettings::MediaSourceEnabled,                true);
    settings->setAttribute(QWebSettings::LocalStorageEnabled,               true);
    settings->setAttribute(QWebSettings::OfflineStorageDatabaseEnabled,     true);
    settings->setAttribute(QWebSettings::OfflineWebApplicationCacheEnabled, true);

    settings->setFontFamily(QWebSettings::StandardFont,       m_settings.value(QStringLiteral("StandardFont")).toString());
    settings->setFontFamily(QWebSettings::SerifFont,          m_settings.value(QStringLiteral("SerifFont")).toString());
    settings->setFontFamily(QWebSettings::SansSerifFont,      m_settings.value(QStringLiteral("SansSerifFont")).toString());
    settings->setFontFamily(QWebSettings::CursiveFont,        m_settings.value(QStringLiteral("CursiveFont")).toString());
    settings->setFontFamily(QWebSettings::FantasyFont,        m_settings.value(QStringLiteral("FantasyFont")).toString());
    settings->setFontFamily(QWebSettings::FixedFont,          m_settings.value(QStringLiteral("FixedFont")).toString());

    settings->setFontSize(QWebSettings::DefaultFontSize,      m_settings.value(QStringLiteral("StandardFontSize")).toInt());
    settings->setFontSize(QWebSettings::DefaultFixedFontSize, m_settings.value(QStringLiteral("FixedFontSize")).toInt());

    QDir cachePath(QDir::homePath() + QDir::separator() + QStringLiteral(".cache") + QDir::separator() + QStringLiteral("Vaccarelli"));
    if (!cachePath.exists())
        cachePath.mkpath(cachePath.absolutePath());
    QWebSettings::setIconDatabasePath(cachePath.absolutePath());
    QWebSettings::enablePersistentStorage(cachePath.absolutePath());
    settings->setLocalStoragePath(QString("%1%2%3").arg(cachePath.absolutePath()).arg(QDir::separator()).arg(QStringLiteral("LocalStorage")));
    settings->setOfflineStoragePath(QString("%1%2%3").arg(cachePath.absolutePath()).arg(QDir::separator()).arg(QStringLiteral("OfflineStorage")));
}

void Settings::setDefaults()
{
    m_firstRun = true;

    QString storageFolder = QFileInfo(m_settings.fileName()).absoluteDir().absolutePath() + QDir::separator();
    m_settings.setValue(QStringLiteral("StoragePath"), storageFolder);
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

    QWebSettings *webSettings = QWebSettings::globalSettings();
    m_settings.setValue(QStringLiteral("StandardFont"), webSettings->fontFamily(QWebSettings::StandardFont));
    m_settings.setValue(QStringLiteral("SerifFont"), webSettings->fontFamily(QWebSettings::SerifFont));
    m_settings.setValue(QStringLiteral("SansSerifFont"), webSettings->fontFamily(QWebSettings::SansSerifFont));
    m_settings.setValue(QStringLiteral("CursiveFont"), webSettings->fontFamily(QWebSettings::CursiveFont));
    m_settings.setValue(QStringLiteral("FantasyFont"), webSettings->fontFamily(QWebSettings::FantasyFont));
    m_settings.setValue(QStringLiteral("FixedFont"), webSettings->fontFamily(QWebSettings::FixedFont));

    m_settings.setValue(QStringLiteral("StandardFontSize"), webSettings->fontSize(QWebSettings::DefaultFontSize));
    m_settings.setValue(QStringLiteral("FixedFontSize"), webSettings->fontSize(QWebSettings::DefaultFixedFontSize));

    // Todo: Store settings such as whether or not to store history
}
