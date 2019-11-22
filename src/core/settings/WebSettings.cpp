#include "CookieJar.h"
#include "Settings.h"
#include "UserAgentManager.h"
#include "WebSettings.h"

#include <QDir>
#include <QString>
#include <QWebEngineProfile>
#include <QWebEngineSettings>
#include <QtGlobal>
#include <QtWebEngineCoreVersion>

WebSettings::WebSettings(const ViperServiceLocator &serviceLocator, QWebEngineSettings *webEngineSettings, QWebEngineProfile *webEngineProfile, QWebEngineProfile *privateProfile) :
    QObject(nullptr),
    m_cookieJar(nullptr),
    m_userAgentManager(nullptr),
    m_webEngineSettings(webEngineSettings),
    m_webEngineProfile(webEngineProfile),
    m_privateWebProfile(privateProfile)
{
    m_cookieJar = serviceLocator.getServiceAs<CookieJar>("CookieJar");
    m_userAgentManager = serviceLocator.getServiceAs<UserAgentManager>("UserAgentManager");

    if (Settings *settings = serviceLocator.getServiceAs<Settings>("Settings"))
    {
        init(settings);

        connect(settings, &Settings::settingChanged, this, &WebSettings::onSettingChanged);
    }
}

WebSettings::~WebSettings()
{
}

void WebSettings::onSettingChanged(BrowserSetting setting, const QVariant &value)
{
    switch (setting)
    {
        case BrowserSetting::EnableJavascript:
            m_webEngineSettings->setAttribute(QWebEngineSettings::JavascriptEnabled, value.toBool());
            break;
        case BrowserSetting::EnableJavascriptPopups:
            m_webEngineSettings->setAttribute(QWebEngineSettings::JavascriptCanOpenWindows, value.toBool());
            break;
        case BrowserSetting::AutoLoadImages:
            m_webEngineSettings->setAttribute(QWebEngineSettings::AutoLoadImages, value.toBool());
            break;
        case BrowserSetting::EnablePlugins:
            m_webEngineSettings->setAttribute(QWebEngineSettings::PluginsEnabled, value.toBool());
            break;
        case BrowserSetting::EnableCookies:
        {
            if (m_cookieJar)
                m_cookieJar->setCookiesEnabled(value.toBool());
            break;
        }
        case BrowserSetting::EnableThirdPartyCookies:
        {
            if (m_cookieJar)
                m_cookieJar->setThirdPartyCookiesEnabled(value.toBool());
            break;
        }
        case BrowserSetting::CookiesDeleteWithSession:
        {
            QWebEngineProfile::PersistentCookiesPolicy cookiesPolicy =
                    value.toBool() ? QWebEngineProfile::NoPersistentCookies : QWebEngineProfile::AllowPersistentCookies;
            m_webEngineProfile->setPersistentCookiesPolicy(cookiesPolicy);
            break;
        }
        case BrowserSetting::EnableXSSAudit:
            m_webEngineSettings->setAttribute(QWebEngineSettings::XSSAuditingEnabled, value.toBool());
            break;
        case BrowserSetting::CustomUserAgent:
        {
            if (!m_userAgentManager)
                break;

            const QString userAgentValue = value.toBool() ? m_userAgentManager->getUserAgent().Value : QString();
            m_webEngineProfile->setHttpUserAgent(userAgentValue);

            if (m_privateWebProfile)
                m_privateWebProfile->setHttpUserAgent(userAgentValue);

            break;
        }
        case BrowserSetting::ScrollAnimatorEnabled:
            m_webEngineSettings->setAttribute(QWebEngineSettings::ScrollAnimatorEnabled, value.toBool());
            break;
        case BrowserSetting::StandardFont:
            m_webEngineSettings->setFontFamily(QWebEngineSettings::StandardFont, value.toString());
            break;
        case BrowserSetting::SerifFont:
            m_webEngineSettings->setFontFamily(QWebEngineSettings::SerifFont, value.toString());
            break;
        case BrowserSetting::SansSerifFont:
            m_webEngineSettings->setFontFamily(QWebEngineSettings::SansSerifFont, value.toString());
            break;
        case BrowserSetting::CursiveFont:
            m_webEngineSettings->setFontFamily(QWebEngineSettings::CursiveFont, value.toString());
            break;
        case BrowserSetting::FantasyFont:
            m_webEngineSettings->setFontFamily(QWebEngineSettings::FantasyFont, value.toString());
            break;
        case BrowserSetting::FixedFont:
            m_webEngineSettings->setFontFamily(QWebEngineSettings::FixedFont, value.toString());
            break;
        case BrowserSetting::StandardFontSize:
            m_webEngineSettings->setFontSize(QWebEngineSettings::DefaultFontSize, value.toInt());
            break;
        case BrowserSetting::FixedFontSize:
            m_webEngineSettings->setFontSize(QWebEngineSettings::DefaultFixedFontSize, value.toInt());
            break;
        default:
            break;
    }
}

void WebSettings::init(Settings *settings)
{
    m_webEngineSettings->setAttribute(QWebEngineSettings::FullScreenSupportEnabled, true);
    m_webEngineSettings->setAttribute(QWebEngineSettings::JavascriptEnabled,        settings->getValue(BrowserSetting::EnableJavascript).toBool());
    m_webEngineSettings->setAttribute(QWebEngineSettings::JavascriptCanOpenWindows, settings->getValue(BrowserSetting::EnableJavascriptPopups).toBool());
    m_webEngineSettings->setAttribute(QWebEngineSettings::AutoLoadImages,           settings->getValue(BrowserSetting::AutoLoadImages).toBool());
    m_webEngineSettings->setAttribute(QWebEngineSettings::PluginsEnabled,           settings->getValue(BrowserSetting::EnablePlugins).toBool());
    m_webEngineSettings->setAttribute(QWebEngineSettings::XSSAuditingEnabled,       settings->getValue(BrowserSetting::EnableXSSAudit).toBool());
    m_webEngineSettings->setAttribute(QWebEngineSettings::ScrollAnimatorEnabled,    settings->getValue(BrowserSetting::ScrollAnimatorEnabled).toBool());
    m_webEngineSettings->setAttribute(QWebEngineSettings::LocalStorageEnabled,      true);

    m_webEngineSettings->setFontFamily(QWebEngineSettings::StandardFont,       settings->getValue(BrowserSetting::StandardFont).toString());
    m_webEngineSettings->setFontFamily(QWebEngineSettings::SerifFont,          settings->getValue(BrowserSetting::SerifFont).toString());
    m_webEngineSettings->setFontFamily(QWebEngineSettings::SansSerifFont,      settings->getValue(BrowserSetting::SansSerifFont).toString());
    m_webEngineSettings->setFontFamily(QWebEngineSettings::CursiveFont,        settings->getValue(BrowserSetting::CursiveFont).toString());
    m_webEngineSettings->setFontFamily(QWebEngineSettings::FantasyFont,        settings->getValue(BrowserSetting::FantasyFont).toString());
    m_webEngineSettings->setFontFamily(QWebEngineSettings::FixedFont,          settings->getValue(BrowserSetting::FixedFont).toString());

    m_webEngineSettings->setFontSize(QWebEngineSettings::DefaultFontSize,      settings->getValue(BrowserSetting::StandardFontSize).toInt());
    m_webEngineSettings->setFontSize(QWebEngineSettings::DefaultFixedFontSize, settings->getValue(BrowserSetting::FixedFontSize).toInt());

    if (m_cookieJar != nullptr)
    {
        m_cookieJar->setCookiesEnabled(settings->getValue(BrowserSetting::EnableCookies).toBool());
        m_cookieJar->setThirdPartyCookiesEnabled(settings->getValue(BrowserSetting::EnableThirdPartyCookies).toBool());
    }

    QWebEngineProfile::PersistentCookiesPolicy cookiesPolicy = QWebEngineProfile::AllowPersistentCookies;
    if (settings->getValue(BrowserSetting::CookiesDeleteWithSession).toBool())
        cookiesPolicy = QWebEngineProfile::NoPersistentCookies;

    m_webEngineProfile->setPersistentCookiesPolicy(cookiesPolicy);

    QString cachePath =
            QString("%1%2%3%2%4").arg(QDir::homePath()).arg(QDir::separator()).arg(QLatin1String(".cache")).arg(QLatin1String("Vaccarelli"));
    QDir cacheDir(cachePath);
    if (!cacheDir.exists())
        cacheDir.mkpath(cacheDir.absolutePath());

    m_webEngineProfile->setCachePath(cacheDir.absolutePath());
    m_webEngineProfile->setPersistentStoragePath(cacheDir.absolutePath());

    // Check if custom user agent is used
    if (settings->getValue(BrowserSetting::CustomUserAgent).toBool())
    {
        if (m_userAgentManager != nullptr)
        {
            m_webEngineProfile->setHttpUserAgent(m_userAgentManager->getUserAgent().Value);

            if (m_privateWebProfile)
                m_privateWebProfile->setHttpUserAgent(m_userAgentManager->getUserAgent().Value);
        }
    }

#if (QT_VERSION >= QT_VERSION_CHECK(5, 13, 0))
    m_webEngineSettings->setAttribute(QWebEngineSettings::PdfViewerEnabled, true);
#endif
}
