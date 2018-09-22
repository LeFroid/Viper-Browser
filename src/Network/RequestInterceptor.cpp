#include "AdBlockManager.h"
#include "RequestInterceptor.h"
#include "Settings.h"

#include <QWebEngineUrlRequestInfo>

RequestInterceptor::RequestInterceptor(QObject *parent) :
    QWebEngineUrlRequestInterceptor(parent)
{
}

void RequestInterceptor::setSettings(Settings *settings)
{
    m_settings = settings;
}

void RequestInterceptor::interceptRequest(QWebEngineUrlRequestInfo &info)
{
    const QString requestScheme = info.requestUrl().scheme();
    if (requestScheme != QLatin1String("viper")
            && requestScheme != QLatin1String("blocked")
            && info.requestMethod() == QString("GET"))
    {
        if (AdBlockManager::instance().shouldBlockRequest(info))
            info.block(true);
    }

    // Check if do not track setting enabled, if so, send header DNT with value 1
    if (m_settings && m_settings->getValue(BrowserSetting::SendDoNotTrack).toBool())
        info.setHttpHeader("DNT", "1");
}
