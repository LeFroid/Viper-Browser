#include "BlockedNetworkReply.h"
#include "BrowserApplication.h"
#include "Settings.h"

#include <QFile>
#include <QTextStream>
#include <QTimer>

BlockedNetworkReply::BlockedNetworkReply(const QNetworkRequest &request, const QString &filter, QObject *parent) :
    QNetworkReply(parent)
{
    setOperation(QNetworkAccessManager::GetOperation);
    setRequest(request);
    setUrl(request.url());
    setError(QNetworkReply::ContentAccessDenied, tr("Advertisement has been blocked by the rule: %1").arg(filter));
    QTimer::singleShot(0, this, SLOT(delayedFinished()));
}

qint64 BlockedNetworkReply::readData(char *data, qint64 maxSize)
{
    Q_UNUSED(data);
    Q_UNUSED(maxSize);
    return -1;
}

void BlockedNetworkReply::delayedFinished()
{
    emit error(QNetworkReply::ContentAccessDenied);
    emit finished();
    deleteLater();
}
