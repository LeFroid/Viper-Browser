#include "AdBlocker.h"
#include "NetworkAccessManager.h"

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
        AdBlocker &adBlock = AdBlocker::instance();
        if (adBlock.isBlocked(request.url().host()))
            return adBlock.getBlockedReply(request);
    }
    return QNetworkAccessManager::createRequest(op, request, outgoingData);
}
