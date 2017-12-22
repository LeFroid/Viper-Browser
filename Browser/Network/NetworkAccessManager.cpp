#include "AdBlocker.h"
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

        /*
        AdBlocker &adBlock = AdBlocker::instance();
        BlockedNetworkReply *blockedReply = adBlock.getBlockedReply(request);
        if (blockedReply != nullptr)
            return blockedReply;*/
    }
    return QNetworkAccessManager::createRequest(op, request, outgoingData);
}
