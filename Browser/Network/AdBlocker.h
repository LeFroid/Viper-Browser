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

/**
 * @class AdBlocker
 * @brief The AdBlocker class parses a list of blocked hosts known for their advertisements,
 *        and prevents the \ref NetworkAccessManager from loading any of these ads
 */
class AdBlocker : public QObject
{
    Q_OBJECT

public:
    /// Constructs the ad blocker
    AdBlocker(QObject *parent = nullptr);

    /// Static adblocker instance
    static AdBlocker &instance();

    /// Returns true if the given host is blocked, false if they are not blocked
    bool isBlocked(const QString &host) const;

    /// Creates and returns a network reply indicating that the GET request is not allowed to go through
    BlockedNetworkReply *getBlockedReply(const QNetworkRequest &request);

private:
    /// Loads the list of hosts to block when calling createRequest()
    void loadAdBlockList();

private:
    /// Set of hosts to block
    QSet<QString> m_blockedHosts;
};

#endif // ADBLOCKER_H
