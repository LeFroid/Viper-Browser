#include "BrowserApplication.h"
#include "InternalDownloadItem.h"
#include "DownloadManager.h"
#include "NetworkAccessManager.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QNetworkReply>

#include <QDebug>

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
    // Create parent directory if does not exist
    QDir parentDir(m_downloadDir);
    if (!parentDir.exists())
    {
        if (!parentDir.mkpath(parentDir.absolutePath()))
            return;
    }

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
    const QString url = m_reply->url().toString();

    // Request file name for download if needed
    QString fileName = QDir(m_downloadDir).filePath(url.mid(url.lastIndexOf(QChar('/')) + 1));

    if (!m_writeOverExisting)
    {
        const QString suffix = url.mid(url.lastIndexOf(QChar('.')) + 1);
        fileName = getDefaultFileName(fileName.left(fileName.lastIndexOf(QChar('.'))), suffix);
    }

    // Create file on disk
    m_file.setFileName(fileName);

    if (m_reply->isFinished() || m_reply->error() != QNetworkReply::NoError)
        onFinished();
}

void InternalDownloadItem::onReadyRead()
{
    // Open the output file or die trying
    if (!m_file.isOpen() && !m_file.open(QIODevice::WriteOnly))
    {
        m_reply->abort();
        return;
    }

    if (m_file.write(m_reply->readAll()) == -1)
    {
        m_reply->abort();
    }
    else
    {
        m_inProgress = true;
    }
}

void InternalDownloadItem::onFinished()
{
    if (m_finished)
        return;

    m_finished = true;
    const bool hadError = m_reply->error() != QNetworkReply::NoError;
    if (!hadError && (m_file.size() == 0 || m_reply->isReadable()))
    {
        onReadyRead();
    }

    m_file.flush();
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

    if (m_file.isOpen())
    {
        m_file.close();
        m_file.remove();
    }

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
