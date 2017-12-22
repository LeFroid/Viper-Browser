#ifndef ADBLOCKER_H
#define ADBLOCKER_H

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
    BlockedNetworkReply(const QNetworkRequest &request, QObject *parent = nullptr);
    void abort() {}

protected:
    qint64 readData(char *data, qint64 maxSize);

private slots:
    void delayedFinished();
};

#endif // ADBLOCKER_H
