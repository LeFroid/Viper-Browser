#include "AdBlocker.h"
#include "BrowserApplication.h"
#include "Settings.h"

#include <QFile>
#include <QTextStream>
#include <QTimer>
#include <QWebFrame>

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
    // todo: with adblockplus format, perform something like the following (for things like domain-specific blocking rules):
    //QWebFrame *originFrame = qobject_cast<QWebFrame*>(request.originatingObject());
    //if (originFrame && isRequestBlocked(request, originFrame->baseUrl())) { return BlockedNetworkReply(request, this); }
    // keep a container of blacklisted domains and content types, etc., and another container of whitelisted sources for the ABP exception rules

    if (m_blockedHosts.contains(request.url().host()))
        return new BlockedNetworkReply(request, this);
    return nullptr;
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
