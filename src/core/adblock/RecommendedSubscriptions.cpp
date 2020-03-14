#include "RecommendedSubscriptions.h"

#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>

namespace adblock
{

RecommendedSubscriptions::RecommendedSubscriptions() :
    std::vector<std::pair<QString, QUrl>>()
{
    load();
}

void RecommendedSubscriptions::load()
{
    if (!empty())
        return;

    QFile f(QStringLiteral(":/adblock_recommended.json"));
    if (!f.exists() || !f.open(QIODevice::ReadOnly))
        return;

    QByteArray subData = f.readAll();
    f.close();

    // Parse the JSON object for recommended subscription objects
    QJsonDocument subscriptionDoc(QJsonDocument::fromJson(subData));
    QJsonObject subscriptionObj = subscriptionDoc.object();
    for (auto it = subscriptionObj.begin(); it != subscriptionObj.end(); ++it)
    {
        const QString name = it.key();
        const QJsonObject itemInfo = it.value().toObject();
        push_back(std::make_pair(name, QUrl(itemInfo.value(QLatin1String("source")).toString())));
    }
}

}
