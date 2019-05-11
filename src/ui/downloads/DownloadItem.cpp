#include "BrowserApplication.h"
#include "CommonUtil.h"
#include "DownloadItem.h"
#include "ui_DownloadItem.h"
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
#include <QStyle>
#include <QtGlobal>
#include <QtWebEngineCoreVersion>

QMimeDatabase mimeDB;

DownloadItem::DownloadItem(QWebEngineDownloadItem *item, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::DownloadItem),
    m_download(item),
    m_downloadDir(),
    m_bytesReceived(0),
    m_pushButtonPauseResume(nullptr)
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

bool DownloadItem::isFinished() const
{
    return m_download->isFinished();
}

void DownloadItem::cancel()
{
    m_download->cancel();
}

void DownloadItem::contextMenuEvent(QContextMenuEvent *event)
{
    QMenu menu;

    if (!m_download->isFinished())
    {
#if (QTWEBENGINECORE_VERSION >= QT_VERSION_CHECK(5, 10, 0))
        if (m_download->isPaused())
            menu.addAction(tr("Resume download"), m_download, &QWebEngineDownloadItem::resume);
        else
            menu.addAction(tr("Pause download"), m_download, &QWebEngineDownloadItem::pause);

        menu.addSeparator();
#endif
        menu.addAction(tr("Cancel download"), m_download, &QWebEngineDownloadItem::cancel);
    }
    else
    {
        menu.addAction(tr("Open in folder"),   this, &DownloadItem::openDownloadFolder);
        menu.addAction(tr("Remove from list"), this, &DownloadItem::removeFromList);
    }
    menu.exec(mapToGlobal(event->pos()));
}

void DownloadItem::setupItem()
{
    ui->labelDownloadName->setText(QFileInfo(m_download->path()).fileName());
    ui->labelDownloadSize->setText(QString());
    ui->progressBarDownload->show();
    ui->labelDownloadSource->setText(m_download->url().toDisplayString(QUrl::RemoveScheme | QUrl::RemoveFilename | QUrl::StripTrailingSlash).mid(2));

    ui->pushButtonOpenFolder->hide();

#if (QTWEBENGINECORE_VERSION >= QT_VERSION_CHECK(5, 10, 0))
    m_pushButtonPauseResume = new QPushButton(QIcon::fromTheme(QLatin1String("media-playback-pause")), QString(), this);
    m_pushButtonPauseResume->setFlat(true);
    ui->horizontalLayout->addWidget(m_pushButtonPauseResume, 0, Qt::AlignRight);

    connect(m_pushButtonPauseResume, &QPushButton::clicked, this, [this](){
        if (m_download->state() != QWebEngineDownloadItem::DownloadInProgress)
            return;

        if (m_download->isPaused())
            m_download->resume();
        else
            m_download->pause();
    });

    connect(m_download, &QWebEngineDownloadItem::isPausedChanged, this, [this](bool isPaused){
        if (m_download->state() != QWebEngineDownloadItem::DownloadInProgress)
            return;

        const QLatin1String iconName
                = isPaused ? QLatin1String("media-playback-start") : QLatin1String("media-playback-pause");
        m_pushButtonPauseResume->setIcon(QIcon::fromTheme(iconName));

        const QString tooltipText = isPaused ? tr("Resume download") : tr("Pause download");
        m_pushButtonPauseResume->setToolTip(tooltipText);
    });
#endif


    // Setup network slots
    connect(m_download, &QWebEngineDownloadItem::downloadProgress, this, &DownloadItem::onDownloadProgress);
    connect(m_download, &QWebEngineDownloadItem::finished,         this, &DownloadItem::onFinished);
    connect(m_download, &QWebEngineDownloadItem::stateChanged,     this, &DownloadItem::onStateChanged);

    // Set icon for the download item
    setIconForItem(QFileInfo(m_download->path()).fileName());

    if (m_download->state() == QWebEngineDownloadItem::DownloadCompleted)
        onFinished();
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

    qint64 downloadPct = 100 * (static_cast<float>(bytesReceived) / bytesTotal);
    ui->progressBarDownload->setValue(downloadPct);

    // Update download file size label
    ui->labelDownloadSize->setText(tr("%1 of %2").arg(CommonUtil::bytesToUserFriendlyStr(bytesReceived)).arg(CommonUtil::bytesToUserFriendlyStr(bytesTotal)));
}

void DownloadItem::onFinished()
{
    ui->progressBarDownload->setValue(100);
    ui->progressBarDownload->setDisabled(true);
    ui->labelDownloadSize->setText(CommonUtil::bytesToUserFriendlyStr(m_bytesReceived));

    if (m_pushButtonPauseResume)
        m_pushButtonPauseResume->hide();

    ui->pushButtonOpenFolder->show();
}

void DownloadItem::onStateChanged(QWebEngineDownloadItem::DownloadState state)
{
    if (state == QWebEngineDownloadItem::DownloadCancelled)
    {
        if (m_pushButtonPauseResume)
            m_pushButtonPauseResume->hide();

        ui->progressBarDownload->setValue(0);
        ui->progressBarDownload->setDisabled(true);
        ui->labelDownloadSize->setText(tr("Cancelled - %1 downloaded").arg(CommonUtil::bytesToUserFriendlyStr(m_bytesReceived)));
    }
    else if (state == QWebEngineDownloadItem::DownloadInterrupted)
    {
        if (m_pushButtonPauseResume)
            m_pushButtonPauseResume->hide();

        ui->progressBarDownload->setValue(0);
        ui->progressBarDownload->setDisabled(true);
        ui->labelDownloadSize->setText(tr("Interrupted - %1").arg(m_download->interruptReasonString()));
    }
}

void DownloadItem::openDownloadFolder()
{
    QString folderUrlStr = QString("file://%1").arg(m_downloadDir);
    static_cast<void>(QDesktopServices::openUrl(QUrl(folderUrlStr, QUrl::TolerantMode)));
}
