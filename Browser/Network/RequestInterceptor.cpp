#include "AdBlockManager.h"
#include "RequestInterceptor.h"

#include <QWebEngineUrlRequestInfo>

RequestInterceptor::RequestInterceptor(QObject *parent) :
    QWebEngineUrlRequestInterceptor(parent)
{
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
}
