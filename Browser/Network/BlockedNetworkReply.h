#ifndef BLOCKEDNETWORKREPLY_H
#define BLOCKEDNETWORKREPLY_H

#include <QNetworkReply>
#include <QNetworkRequest>
#include <QObject>
#include <QSet>
#include <QString>

/**
 * @class BlockedNetworkReply
 * @brief This class is used to ignore GET requests that are included in the AdBlock list
 */
class BlockedNetworkReply : public QNetworkReply
{
    Q_OBJECT

public:
    /// Constructs the ad-block network reply, given the original request, filter string and a parent object
    BlockedNetworkReply(const QNetworkRequest &request, const QString &filter, QObject *parent = nullptr);
    void abort() {}

protected:
    qint64 readData(char *data, qint64 maxSize);

private slots:
    void delayedFinished();
};

#endif // BLOCKEDNETWORKREPLY_H
