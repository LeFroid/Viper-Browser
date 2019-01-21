#ifndef NETWORKACCESSMANAGER_H
#define NETWORKACCESSMANAGER_H

#include <QList>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QString>

/**
 * @class NetworkAccessManager
 * @brief Handles all network requests - defaulting to the \ref QNetworkAccessManager
 *        implementation for all requests with the exception of a few types of activity
 *        such as the blocking of unwanted ads (via \ref AdBlockManager class)
 */
class NetworkAccessManager : public QNetworkAccessManager
{
    Q_OBJECT

public:
    /// Constructs the network access manager, optionally setting a parent object
    NetworkAccessManager(QObject *parent = nullptr);

protected:
    virtual QNetworkReply *createRequest(Operation op, const QNetworkRequest &request, QIODevice *outgoingData) override;
};

#endif // NETWORKACCESSMANAGER_H
