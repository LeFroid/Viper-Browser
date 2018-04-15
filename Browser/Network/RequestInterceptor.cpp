#include "AdBlockManager.h"
#include "RequestInterceptor.h"

#include <QWebEngineUrlRequestInfo>

RequestInterceptor::RequestInterceptor(QObject *parent) :
    QWebEngineUrlRequestInterceptor(parent)
{
}

void RequestInterceptor::interceptRequest(QWebEngineUrlRequestInfo &info)
{
    if (info.requestMethod() == QString("GET"))
    {
        if (AdBlockManager::instance().shouldBlockRequest(info))
            info.block(true);
    }
}
