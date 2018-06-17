#include "BrowserApplication.h"
#include "DownloadItem.h"
#include "ui_downloaditem.h"
#include "DownloadManager.h"
#include "NetworkAccessManager.h"

#include <QContextMenuEvent>
#include <QDesktopServices>
#include <QDir>
#include <QFileInfo>
#include <QList>
#include <QMenu>
#include <QMimeDatabase>
#include <QMimeType>
#include <QNetworkReply>

QMimeDatabase mimeDB;

DownloadItem::DownloadItem(QWebEngineDownloadItem *item, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::DownloadItem),
    m_download(item),
    m_downloadDir(),
    m_bytesReceived(0),
    m_inProgress(false)
{
    ui->setupUi(this);

    m_downloadDir = QFileInfo(m_download->path()).absoluteDir().absolutePath();

    // Connect "Open download folder" button to slot
    connect(ui->pushButtonOpenFolder, &QPushButton::clicked, this, &DownloadItem::openDownloadFolder);

    setupItem();
}

DownloadItem::~DownloadItem()
{
    delete ui;
}

void DownloadItem::contextMenuEvent(QContextMenuEvent *event)
{
    QMenu menu;
    menu.addAction(tr("Open in folder"), this, &DownloadItem::openDownloadFolder);
    menu.addAction(tr("Remove from list"), this, &DownloadItem::removeFromList);
    menu.exec(mapToGlobal(event->pos()));
}

void DownloadItem::setupItem()
{
    m_inProgress = false;

    ui->labelDownloadName->setText(QFileInfo(m_download->path()).fileName());
    ui->labelDownloadSize->setText(QString());
    ui->progressBarDownload->show();
    ui->labelDownloadSource->setText(m_download->url().toDisplayString(QUrl::RemoveScheme | QUrl::RemoveFilename | QUrl::StripTrailingSlash));
    ui->pushButtonOpenFolder->hide();

    // Setup network slots
    connect(m_download, &QWebEngineDownloadItem::downloadProgress, this, &DownloadItem::onDownloadProgress);
    connect(m_download, &QWebEngineDownloadItem::finished, this, &DownloadItem::onFinished);

    // Set icon for the download item
    setIconForItem(QFileInfo(m_download->path()).fileName());

    if (m_download->state() == QWebEngineDownloadItem::DownloadCompleted)
    {
        onFinished();
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
    ui->progressBarDownload->setValue(100);
    ui->progressBarDownload->setDisabled(true);
    ui->labelDownloadSize->setText(getUserByteString(m_bytesReceived));
    ui->pushButtonOpenFolder->show();
}

void DownloadItem::onStateChanged(QWebEngineDownloadItem::DownloadState state)
{
    if (state == QWebEngineDownloadItem::DownloadCancelled)
    {
        ui->progressBarDownload->setValue(0);
        ui->progressBarDownload->setDisabled(true);
        ui->labelDownloadSize->setText(QString("Cancelled - %1 downloaded").arg(getUserByteString(m_bytesReceived)));
    }
    else if (state == QWebEngineDownloadItem::DownloadInterrupted)
    {
        ui->progressBarDownload->setValue(0);
        ui->progressBarDownload->setDisabled(true);
        ui->labelDownloadSize->setText(QString("Interrupted - %1").arg(m_download->interruptReasonString()));
    }
}

void DownloadItem::openDownloadFolder()
{
    QString folderUrlStr = QString("file://%1").arg(m_downloadDir);
    static_cast<void>(QDesktopServices::openUrl(QUrl(folderUrlStr, QUrl::TolerantMode)));
}

QString DownloadItem::getUserByteString(qint64 value) const
{
    QString userStr;
    double valDiv = value;

    if (value >= 1099511627776)
    {
        // >= 1 TB
        valDiv /= 1099511627776;
        userStr = QString::number(valDiv, 'f', 2) + " TB";
    }
    else if (value >= 1073741824)
    {
        // >= 1 GB
        valDiv /= 1073741824;
        userStr = QString::number(valDiv, 'f', 2) + " GB";
    }
    else if (value >= 1048576)
    {
        // >= 1 MB
        valDiv /= 1048576;
        userStr = QString::number(valDiv, 'f', 2) + " MB";
    }
    else if (value > 1024)
    {
        // >= 1 KB
        valDiv /= 1024;
        userStr = QString::number(valDiv, 'f', 2) + " KB";
    }
    else
    {
        // < 1 KB
        return QString::number(value) + " bytes";
    }

    return userStr;
}
