#include "AdBlockManager.h"
#include "RequestInterceptor.h"
#include "UserAgentManager.h"
#include "WebPage.h"

#include <QByteArray>
#include <QWebEngineUrlRequestInfo>
#include <QtWebEngineCoreVersion>
#include <QtGlobal>

const static QByteArray cUserAgentHeader = QByteArray("User-Agent");

RequestInterceptor::RequestInterceptor(const ViperServiceLocator &serviceLocator, QObject *parent) :
    QWebEngineUrlRequestInterceptor(parent),
    m_serviceLocator(serviceLocator),
    m_adBlockManager(nullptr),
    m_parentPage(qobject_cast<WebPage*>(parent)),
    m_sendDoNotTrack(false),
    m_sendCustomUserAgent(false),
    m_userAgent()
{
    setObjectName(QStringLiteral("RequestInterceptor"));
}

void RequestInterceptor::fetchServices()
{
    if (Settings *settings = m_serviceLocator.getServiceAs<Settings>("Settings"))
    {
        m_sendDoNotTrack = settings->getValue(BrowserSetting::SendDoNotTrack).toBool();

        if (UserAgentManager *userAgentManager = m_serviceLocator.getServiceAs<UserAgentManager>("UserAgentManager"))
        {
            m_sendCustomUserAgent = settings->getValue(BrowserSetting::CustomUserAgent).toBool();
            m_userAgent = m_sendCustomUserAgent ? userAgentManager->getUserAgent().Value.toLatin1() : QByteArray();
        }

        connect(settings, &Settings::settingChanged, this, &RequestInterceptor::onSettingChanged);
    }

    if (!m_adBlockManager)
        m_adBlockManager = m_serviceLocator.getServiceAs<adblock::AdBlockManager>("AdBlockManager");
}

void RequestInterceptor::interceptRequest(QWebEngineUrlRequestInfo &info)
{
    if (!m_adBlockManager)
        fetchServices();

    const QString requestScheme = info.requestUrl().scheme();
    if (requestScheme != QStringLiteral("viper")
            && requestScheme != QStringLiteral("blocked"))
            // && info.requestMethod() == QString("GET"))
    {
        QUrl firstPartyUrl { info.firstPartyUrl() };

#if (QTWEBENGINECORE_VERSION >= QT_VERSION_CHECK(5, 13, 0))
        if (m_parentPage)
        {
            if (!m_parentPage->url().isEmpty())
                firstPartyUrl = m_parentPage->url();
            else if (!m_parentPage->requestedUrl().isEmpty())
                firstPartyUrl = m_parentPage->requestedUrl();
        }
#endif

        if (m_adBlockManager && m_adBlockManager->shouldBlockRequest(info, firstPartyUrl))
            info.block(true);
    }

    // Check if we need to send the do not track header
    if (m_sendDoNotTrack)
        info.setHttpHeader("DNT", "1");

    if (m_sendCustomUserAgent)
        info.setHttpHeader(cUserAgentHeader, m_userAgent);
}

void RequestInterceptor::onSettingChanged(BrowserSetting setting, const QVariant &value)
{
    if (setting == BrowserSetting::SendDoNotTrack)
        m_sendDoNotTrack = value.toBool();
    else if (setting == BrowserSetting::CustomUserAgent)
    {
        m_sendCustomUserAgent = value.toBool();

        UserAgentManager *userAgentManager = m_serviceLocator.getServiceAs<UserAgentManager>("UserAgentManager");
 
        if (m_sendCustomUserAgent && userAgentManager != nullptr)
            m_userAgent = userAgentManager->getUserAgent().Value.toLatin1();
    }
}
