#include "AdBlockManager.h"
#include "RequestInterceptor.h"
#include "Settings.h"

#include <QWebEngineUrlRequestInfo>

RequestInterceptor::RequestInterceptor(const ViperServiceLocator &serviceLocator, QObject *parent) :
    QWebEngineUrlRequestInterceptor(parent),
    m_settings(nullptr),
    m_serviceLocator(serviceLocator),
    m_adBlockManager(nullptr)
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
    fetchServices();

    const QString requestScheme = info.requestUrl().scheme();
    if (requestScheme != QLatin1String("viper")
            && requestScheme != QLatin1String("blocked")
            && info.requestMethod() == QString("GET"))
    {
        if (m_adBlockManager && m_adBlockManager->shouldBlockRequest(info))
            info.block(true);
    }

    // Check if do not track setting enabled, if so, send header DNT with value 1
    if (m_settings && m_settings->getValue(BrowserSetting::SendDoNotTrack).toBool())
        info.setHttpHeader("DNT", "1");
}
