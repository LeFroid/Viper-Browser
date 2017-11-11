#include "BrowserApplication.h"
#include "DownloadItem.h"
#include "ui_downloaditem.h"
#include "DownloadManager.h"
#include "NetworkAccessManager.h"

#include <QFileDialog>
#include <QFile>
#include <QFileInfo>
#include <QList>
#include <QMimeDatabase>
#include <QMimeType>
#include <QNetworkReply>

QMimeDatabase mimeDB;

DownloadItem::DownloadItem(QNetworkReply *reply, const QString &downloadDir, bool askForFileName, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::DownloadItem),
    m_reply(reply),
    m_askForFileName(askForFileName),
    m_downloadDir(downloadDir),
    m_bytesReceived(0),
    m_file(),
    m_inProgress(false),
    m_finished(false)
{
    ui->setupUi(this);

    if (m_reply)
        setupItem();
}

DownloadItem::~DownloadItem()
{
    delete ui;
}

void DownloadItem::setupItem()
{
    m_inProgress = false;
    m_finished = false;
    m_reply->setParent(this);

    ui->labelDownloadName->setText(QString());
    ui->labelDownloadSize->setText(QString());
    ui->progressBarDownload->show();

    // Setup network slots
    connect(m_reply, &QNetworkReply::readyRead, this, &DownloadItem::onReadyRead);
    connect(m_reply, &QNetworkReply::downloadProgress, this, &DownloadItem::onDownloadProgress);
    connect(m_reply, &QNetworkReply::finished, this, &DownloadItem::onFinished);
    connect(m_reply, static_cast<void(QNetworkReply::*)(QNetworkReply::NetworkError)>(&QNetworkReply::error), this, &DownloadItem::onError);

    // Get file info
    QString urlPath = m_reply->url().path();
    QFileInfo info(urlPath);
    QString externalName = info.baseName();

    // Request file name for download if needed
    QString fileNameDefault = getDefaultFileName(m_downloadDir + (externalName.isEmpty() ? "unknown" : externalName), info.completeSuffix());
    QString fileName = fileNameDefault;
    if (m_askForFileName)
    {
        fileName = QFileDialog::getSaveFileName(this, tr("Save File As..."), fileNameDefault);
        if (fileName.isEmpty())
        {
            m_reply->close();
            ui->labelDownloadName->setText(fileNameDefault + " - Canceled");
            return;
        }
        // Update download directory in manager class
        QString newDownloadDir = QFileInfo(fileName).absoluteDir().absolutePath();
        sBrowserApplication->getDownloadManager()->setDownloadDir(newDownloadDir);
    }

    // Create file on disk
    m_file.setFileName(fileName);
    QFileInfo localFileInfo(m_file.fileName());

    // Set icon for the download item
    setIconForItem(localFileInfo.fileName());

    // Set file name label
    ui->labelDownloadName->setText(localFileInfo.fileName());

    // Create parent directory if does not exist
    QDir parentDir = localFileInfo.dir();
    if (!parentDir.exists())
    {
        if (!parentDir.mkpath(parentDir.absolutePath()))
        {
            ui->labelDownloadSize->setText(tr("Canceled - Unable to create download directory"));
            return;
        }
    }

    if (m_askForFileName)
        onReadyRead();

    if (m_reply->error() != QNetworkReply::NoError)
    {
        onFinished();
        onError(m_reply->error());
    }
}

void DownloadItem::setIconForItem(const QString &fileName)
{
    QList<QMimeType> mimeTypes = mimeDB.mimeTypesForFileName(fileName);

    // Attempt to set to an appropriate icon, defaulting to standard file icon if none here will work
    QIcon icon;
    for (const auto &mimeType : mimeTypes)
    {
        icon = QIcon::fromTheme(mimeType.iconName());
        if (!icon.isNull())
        {
            ui->labelFileTypeIcon->setPixmap(icon.pixmap(48, 48));
            return;
        }
    }

    ui->labelFileTypeIcon->setPixmap(style()->standardIcon(QStyle::SP_FileIcon).pixmap(48, 48));
}

void DownloadItem::onReadyRead()
{
    if (m_askForFileName && m_file.fileName().isEmpty())
        return;

    // Check if output file is not opened yet
    if (!m_file.isOpen())
    {
        if (!m_file.open(QIODevice::WriteOnly))
        {
            ui->labelDownloadSize->setText("Error opening output file");
            //stop();
            m_reply->abort();
            return;
        }
    }

    if (m_file.write(m_reply->readAll()) == -1)
    {
        ui->labelDownloadSize->setText("Error saving file");
        m_reply->abort();
    }
    else
    {
        m_inProgress = true;
        if (m_finished)
            onFinished();
    }
}

void DownloadItem::onDownloadProgress(qint64 bytesReceived, qint64 bytesTotal)
{
    m_bytesReceived = bytesReceived;

    qint64 downloadPct = 100 * ((float)bytesReceived / bytesTotal);
    ui->progressBarDownload->setValue(downloadPct);

    // Update download file size label
    ui->labelDownloadSize->setText(QString("%1 of %2").arg(getUserByteString(bytesReceived)).arg(getUserByteString(bytesTotal)));
}

void DownloadItem::onFinished()
{
    m_finished = true;
    ui->progressBarDownload->hide();
    QString urlString = m_reply->url().toDisplayString(QUrl::RemoveScheme | QUrl::RemoveFilename | QUrl::StripTrailingSlash);
    ui->labelDownloadSize->setText(QString("%1 - %2").arg(getUserByteString(m_bytesReceived)).arg(urlString));

    m_file.close();
}

void DownloadItem::onError(QNetworkReply::NetworkError /*errorCode*/)
{
    ui->labelDownloadSize->setText(QString("Network error: %1").arg(m_reply->errorString()));
}

void DownloadItem::onMetaDataChanged()
{
    QVariant locHeader = m_reply->header(QNetworkRequest::LocationHeader);
    if (!locHeader.isValid())
        return;

    m_reply->deleteLater();
    m_reply = sBrowserApplication->getNetworkAccessManager()->get(QNetworkRequest(locHeader.toUrl()));
    setupItem();
}

QString DownloadItem::getDefaultFileName(const QString &pathWithoutSuffix, const QString &completeSuffix) const
{
    int attempts = 0;
    QString fileAttempt = pathWithoutSuffix + '.' + completeSuffix;

    while (QFile::exists(fileAttempt))
    {
        // Attempt to avoid conflicts, but don't try forever
        if (attempts > 1000000)
            return fileAttempt;

        fileAttempt = pathWithoutSuffix + " (" + QString::number(++attempts) + ")." + completeSuffix;
    }
    return fileAttempt;
}

QString DownloadItem::getUserByteString(qint64 value) const
{
    QString userStr;
    double valDiv;

    if (value >= 1099511627776)
    {
        // >= 1 TB
        valDiv = value / 1099511627776;
        userStr = QString::number(valDiv, 'g', 2) + " TB";
    }
    else if (value >= 1073741824)
    {
        // >= 1 GB
        valDiv = value / 1073741824;
        userStr = QString::number(valDiv, 'g', 2) + " GB";
    }
    else if (value >= 1048576)
    {
        // >= 1 MB
        valDiv = value / 1048576;
        userStr = QString::number(valDiv, 'g', 2) + " MB";
    }
    else if (value > 1024)
    {
        // >= 1 KB
        valDiv = value / 1024;
        userStr = QString::number(valDiv, 'g', 2) + " KB";
    }
    else
    {
        // < 1 KB
        return QString::number(value) + " bytes";
    }

    return userStr;
}
