#include "AdBlocker.h"
#include "BrowserApplication.h"
#include "Settings.h"

#include <QFile>
#include <QTextStream>
#include <QTimer>

BlockedNetworkReply::BlockedNetworkReply(const QNetworkRequest &request, QObject *parent) :
    QNetworkReply(parent)
{
    setOperation(QNetworkAccessManager::GetOperation);
    setRequest(request);
    setUrl(request.url());
    setError(QNetworkReply::ContentAccessDenied, tr("Advertisement has been blocked"));
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

AdBlocker::AdBlocker(QObject *parent) :
    QObject(parent)
{
    loadAdBlockList();
}

AdBlocker &AdBlocker::instance()
{
    static AdBlocker adBlockInstance;
    return adBlockInstance;
}

bool AdBlocker::isBlocked(const QString &host) const
{
    return m_blockedHosts.contains(host);
}

BlockedNetworkReply *AdBlocker::getBlockedReply(const QNetworkRequest &request)
{
    return new BlockedNetworkReply(request, this);
}

void AdBlocker::loadAdBlockList()
{
    QFile file(sBrowserApplication->getSettings()->getPathValue("AdBlockFile"));
    if (!file.exists())
        return;

    if (file.open(QIODevice::ReadOnly))
    {
        QTextStream input(&file);
        while (!input.atEnd())
            m_blockedHosts.insert(input.readLine());
        file.close();
    }
}
