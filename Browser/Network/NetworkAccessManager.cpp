#include "BlockedNetworkReply.h"
#include "NetworkAccessManager.h"
#include "ViperNetworkReply.h"

#include "AdBlockManager.h"

#include <QNetworkRequest>
#include <QUrl>

NetworkAccessManager::NetworkAccessManager(QObject *parent) :
    QNetworkAccessManager(parent)
{
}

QNetworkReply *NetworkAccessManager::createRequest(NetworkAccessManager::Operation op, const QNetworkRequest &request, QIODevice *outgoingData)
{
    if (op == NetworkAccessManager::GetOperation)
    {
        if (request.url().scheme().compare("viper") == 0)
            return new ViperNetworkReply(request, this);

        AdBlockManager &adBlockMgr = AdBlockManager::instance();
        BlockedNetworkReply *blockedReply = adBlockMgr.getBlockedReply(request);
        if (blockedReply != nullptr)
            return blockedReply;
    }
    else if (op == NetworkAccessManager::PostOperation)
    {
        // Check for existence of content-type header, set to application/x-www-form-urlencoded if not found to prevent warning
        const QByteArray contentTypeHeader("content-type");
        if (!request.hasRawHeader(contentTypeHeader))
        {
            QNetworkRequest modifiedRequest(request);
            modifiedRequest.setRawHeader(contentTypeHeader, QByteArray("application/x-www-form-urlencoded"));
            return QNetworkAccessManager::createRequest(op, modifiedRequest, outgoingData);
        }
    }
    return QNetworkAccessManager::createRequest(op, request, outgoingData);
}
