#include "AdBlocker.h"
#include "NetworkAccessManager.h"
#include "ViperNetworkReply.h"

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

        AdBlocker &adBlock = AdBlocker::instance();
        BlockedNetworkReply *blockedReply = adBlock.getBlockedReply(request);
        if (blockedReply != nullptr)
            return blockedReply;
        /*
         * AdBlockManager &adBlockMgr = AdBlockManager::instance();
         * BlockedNetworkReply *blockedReply = adBlock.getBlockedReply(request);
         * in getBlockedReply:
         *     QUrl baseUrl = qobject_cast<QWebFrame*>(request.originatingObject())->baseUrl();
         *     QUrl requestUrl = request.url();
         *     ElementType elemType = getElementType(request.rawHeader(QByteArray("Accept")));
         *     if (isBlocked(baseUrl, requestUrl, elemType) { return new BlockedNetworkReply(...); } else return nullptr;
         */
    }
    return QNetworkAccessManager::createRequest(op, request, outgoingData);
}
