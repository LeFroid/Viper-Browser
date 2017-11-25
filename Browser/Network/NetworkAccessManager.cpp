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
        if (adBlock.isBlocked(request.url().host()))
            return adBlock.getBlockedReply(request);
    }
    return QNetworkAccessManager::createRequest(op, request, outgoingData);
}
