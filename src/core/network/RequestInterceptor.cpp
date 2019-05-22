#include "AdBlockManager.h"
#include "RequestInterceptor.h"
#include "Settings.h"
#include "WebPage.h"

#include <QWebEngineUrlRequestInfo>
#include <QtWebEngineCoreVersion>
#include <QtGlobal>

RequestInterceptor::RequestInterceptor(const ViperServiceLocator &serviceLocator, QObject *parent) :
    QWebEngineUrlRequestInterceptor(parent),
    m_settings(nullptr),
    m_serviceLocator(serviceLocator),
    m_adBlockManager(nullptr),
    m_parentPage(qobject_cast<WebPage*>(parent))
{
    setObjectName(QLatin1String("RequestInterceptor"));
}

void RequestInterceptor::fetchServices()
{
    if (!m_settings)
    {
        m_settings = m_serviceLocator.getServiceAs<Settings>("Settings");
        connect(m_settings, &Settings::destroyed, this, [this](){
            m_settings = nullptr;
        });
    }
    if (!m_adBlockManager)
        m_adBlockManager = m_serviceLocator.getServiceAs<adblock::AdBlockManager>("AdBlockManager");
}

void RequestInterceptor::interceptRequest(QWebEngineUrlRequestInfo &info)
{
    if (!m_settings || !m_adBlockManager)
        fetchServices();

    const QString requestScheme = info.requestUrl().scheme();
    if (requestScheme != QLatin1String("viper")
            && requestScheme != QLatin1String("blocked")
            && info.requestMethod() == QString("GET"))
    {
        QUrl firstPartyUrl { info.firstPartyUrl() };

#if (QTWEBENGINECORE_VERSION >= QT_VERSION_CHECK(5, 13, 0))
        if (m_parentPage)
            firstPartyUrl = m_parentPage->url();
#endif

        if (m_adBlockManager && m_adBlockManager->shouldBlockRequest(info, firstPartyUrl))
            info.block(true);
    }

    // Check if do not track setting enabled, if so, send header DNT with value 1
    if (m_settings && m_settings->getValue(BrowserSetting::SendDoNotTrack).toBool())
        info.setHttpHeader("DNT", "1");
}
