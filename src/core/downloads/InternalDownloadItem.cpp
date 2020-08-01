#include "BrowserApplication.h"
#include "InternalDownloadItem.h"
#include "DownloadManager.h"
#include "NetworkAccessManager.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QList>
#include <QNetworkReply>


InternalDownloadItem::InternalDownloadItem(QNetworkReply *reply, const QString &downloadDir, bool askForFileName, bool writeOverExisting, QObject *parent) :
    QObject(parent),
    m_reply(reply),
    m_askForFileName(askForFileName),
    m_writeOverExisting(writeOverExisting),
    m_downloadDir(downloadDir),
    m_bytesReceived(0),
    m_file(),
    m_inProgress(false),
    m_finished(false)
{
    if (m_reply != nullptr)
        setupItem();
}

InternalDownloadItem::~InternalDownloadItem()
{
}

void InternalDownloadItem::setupItem()
{
    m_inProgress = false;
    m_finished = false;
    m_reply->setParent(this);

    // Setup network slots
    connect(m_reply, &QNetworkReply::readyRead, this, &InternalDownloadItem::onReadyRead);
    connect(m_reply, &QNetworkReply::finished, this, &InternalDownloadItem::onFinished);
    connect(m_reply, &QNetworkReply::metaDataChanged, this, &InternalDownloadItem::onMetaDataChanged);

    // Get file info
    QFileInfo info(m_reply->url().path());
    QString externalName = info.baseName();

    // Request file name for download if needed
    QString fileNameDefault = QString("%1%2").arg(m_downloadDir).arg(QDir::separator());
    if (!m_writeOverExisting)
    {
        fileNameDefault = getDefaultFileName(QString("%1%2").arg(fileNameDefault, (externalName.isEmpty() ? "unknown" : externalName)),
                                             info.completeSuffix());
    }
    else
        fileNameDefault = QString("%1%2.%3").arg(fileNameDefault, (externalName.isEmpty() ? "unknown" : externalName), info.completeSuffix());

    QString fileName = fileNameDefault;

    // Create file on disk
    m_file.setFileName(fileName);
    QFileInfo localFileInfo(m_file.fileName());

    // Create parent directory if does not exist
    QDir parentDir = localFileInfo.dir();
    if (!parentDir.exists())
    {
        if (!parentDir.mkpath(parentDir.absolutePath()))
            return;
    }

    if (m_askForFileName)
        onReadyRead();

    if (m_reply->error() != QNetworkReply::NoError)
    {
        onFinished();
    }
}

void InternalDownloadItem::onReadyRead()
{
    if (m_askForFileName && m_file.fileName().isEmpty())
        return;

    // Check if output file is not opened yet
    if (!m_file.isOpen())
    {
        if (!m_file.open(QIODevice::WriteOnly))
        {
            m_reply->abort();
            return;
        }
    }

    if (m_file.write(m_reply->readAll()) == -1)
    {
        m_reply->abort();
    }
    else
    {
        m_inProgress = true;
        if (m_finished)
            onFinished();
    }
}

void InternalDownloadItem::onFinished()
{
    const bool hadError = m_reply->error() != QNetworkReply::NoError;

    if (!hadError && (m_file.size() == 0 || (m_bytesReceived > 0 && m_file.size() < m_bytesReceived)))
        onReadyRead();

    m_finished = true;
    m_file.close();

    if (!hadError)
    {
        emit downloadFinished(QFileInfo(m_file).absoluteFilePath());
    }

    m_reply->deleteLater();
}

void InternalDownloadItem::onMetaDataChanged()
{
    QVariant locHeader = m_reply->header(QNetworkRequest::LocationHeader);
    if (!locHeader.isValid())
        return;

    m_reply->deleteLater();
    m_reply = sBrowserApplication->getNetworkAccessManager()->get(QNetworkRequest(locHeader.toUrl()));
    setupItem();
}

QString InternalDownloadItem::getDefaultFileName(const QString &pathWithoutSuffix, const QString &completeSuffix) const
{
    int attempts = 0;
    QString fileAttempt = QString("%1.%2").arg(pathWithoutSuffix, completeSuffix);

    while (QFile::exists(fileAttempt))
    {
        // Attempt to avoid conflicts, but don't try forever
        if (attempts > 1000000)
            return fileAttempt;

        fileAttempt = pathWithoutSuffix + " (" + QString::number(++attempts) + ")." + completeSuffix;
    }
    return fileAttempt;
}
