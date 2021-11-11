#include "BrowserApplication.h"
#include "CommonUtil.h"
#include "DownloadItem.h"
#include "ui_DownloadItem.h"
#include "DownloadManager.h"
#include "NetworkAccessManager.h"

#include <QContextMenuEvent>
#include <QDateTime>
#include <QDesktopServices>
#include <QDir>
#include <QFileInfo>
#include <QList>
#include <QMenu>
#include <QMimeDatabase>
#include <QMimeType>
#include <QNetworkReply>
#include <QStyle>
#include <QTimer>
#include <QtGlobal>
#include <QtWebEngineCoreVersion>

QMimeDatabase mimeDB;

DownloadItem::DownloadItem(QWebEngineDownloadRequest *item, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::DownloadItem),
    m_download(item),
    m_downloadDir(),
    m_bytesReceived(0),
    m_pushButtonPauseResume(nullptr),
    m_startTimeMs(QDateTime::currentMSecsSinceEpoch()),
    m_lastUpdateMs(m_startTimeMs),
    m_timeRemainingMs(0),
    m_countdownTimer(nullptr)
{
    ui->setupUi(this);

    m_downloadDir = m_download->downloadDirectory();

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
        if (m_download->isPaused())
            menu.addAction(tr("Resume download"), m_download, &QWebEngineDownloadRequest::resume);
        else
            menu.addAction(tr("Pause download"), m_download, &QWebEngineDownloadRequest::pause);

        menu.addSeparator();
        menu.addAction(tr("Cancel download"), m_download, &QWebEngineDownloadRequest::cancel);
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
    m_countdownTimer = new QTimer(this);
    connect(m_countdownTimer, &QTimer::timeout, this, &DownloadItem::updateTimeToCompleteLabel);
    m_countdownTimer->start(1000);

    ui->labelDownloadName->setText(m_download->downloadFileName());
    ui->labelDownloadSize->setText(QString());
    ui->labelEstimatedTimeLeft->setText(QString());
    ui->progressBarDownload->show();

    const QString downloadSource = fontMetrics().elidedText(
                m_download->url().toDisplayString(QUrl::RemoveScheme | QUrl::RemoveFilename | QUrl::StripTrailingSlash).mid(2),
                Qt::ElideRight,
                std::max(100, width() - contentsMargins().left() - contentsMargins().right() - 2));
    ui->labelDownloadSource->setText(downloadSource);

    ui->pushButtonOpenFolder->hide();

    m_pushButtonPauseResume = new QPushButton(QIcon::fromTheme(QStringLiteral("media-playback-pause")), QString(), this);
    m_pushButtonPauseResume->setFlat(true);
    ui->horizontalLayout->addWidget(m_pushButtonPauseResume, 0, Qt::AlignRight);

    connect(m_pushButtonPauseResume, &QPushButton::clicked, this, [this](){
        if (m_download->state() != QWebEngineDownloadRequest::DownloadInProgress)
            return;

        if (m_download->isPaused())
            m_download->resume();
        else
            m_download->pause();
    });

    connect(m_download, &QWebEngineDownloadRequest::isPausedChanged, this, [this](){
        if (m_download->state() != QWebEngineDownloadRequest::DownloadInProgress)
            return;

        const bool isPaused = m_download->isPaused();
        const QString iconName
                = isPaused ? QStringLiteral("media-playback-start") : QStringLiteral("media-playback-pause");
        m_pushButtonPauseResume->setIcon(QIcon::fromTheme(iconName));

        const QString tooltipText = isPaused ? tr("Resume download") : tr("Pause download");
        m_pushButtonPauseResume->setToolTip(tooltipText);
    });


    // Setup network slots
    connect(m_download, &QWebEngineDownloadRequest::receivedBytesChanged, this, &DownloadItem::onDownloadProgress);
    connect(m_download, &QWebEngineDownloadRequest::stateChanged,         this, &DownloadItem::onStateChanged);

    // Set icon for the download item
    setIconForItem(m_download->downloadFileName());

    if (m_download->state() == QWebEngineDownloadRequest::DownloadCompleted)
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

void DownloadItem::onDownloadProgress()
{
    const qint64 bytesReceived = m_download->receivedBytes();
    const qint64 bytesTotal = m_download->totalBytes();
    m_bytesReceived = bytesReceived;

    qint64 downloadPct = 100 * (static_cast<float>(bytesReceived) / bytesTotal);
    ui->progressBarDownload->setValue(downloadPct);

    qint64 timeInMs = QDateTime::currentMSecsSinceEpoch();
    qint64 thresholdToUpdateMs = m_timeRemainingMs > 60000 ? 10000 : 4000;

    if (bytesTotal >= 1048576L
            && bytesReceived < bytesTotal
            && (m_lastUpdateMs == m_startTimeMs || timeInMs - m_lastUpdateMs > thresholdToUpdateMs))
    {
        m_lastUpdateMs = timeInMs;
        updateEstimatedTimeLeft(bytesReceived, bytesTotal);
    }

    // Update download file size label
    ui->labelDownloadSize->setText(
                tr("%1 of %2").arg(CommonUtil::bytesToUserFriendlyStr(bytesReceived), CommonUtil::bytesToUserFriendlyStr(bytesTotal)));
}

void DownloadItem::onFinished()
{
    m_timeRemainingMs = 0;
    m_countdownTimer->stop();
    ui->progressBarDownload->setValue(100);
    ui->progressBarDownload->setDisabled(true);
    ui->labelDownloadSize->setText(CommonUtil::bytesToUserFriendlyStr(m_bytesReceived));
    ui->labelEstimatedTimeLeft->setText(QString());

    if (m_pushButtonPauseResume)
        m_pushButtonPauseResume->hide();

    ui->pushButtonOpenFolder->show();
}

void DownloadItem::onStateChanged(QWebEngineDownloadRequest::DownloadState state)
{
    switch (state)
    {
        case QWebEngineDownloadRequest::DownloadRequested:
            break;
        case QWebEngineDownloadRequest::DownloadCancelled:
        {
            m_timeRemainingMs = 0;
            m_countdownTimer->stop();
            if (m_pushButtonPauseResume)
                m_pushButtonPauseResume->hide();

            ui->progressBarDownload->setValue(0);
            ui->progressBarDownload->setDisabled(true);
            ui->labelEstimatedTimeLeft->setText(QString());
            ui->labelDownloadSize->setText(tr("Cancelled - %1 downloaded").arg(CommonUtil::bytesToUserFriendlyStr(m_bytesReceived)));
            break;
        }
        case QWebEngineDownloadRequest::DownloadInterrupted:
        {
            m_timeRemainingMs = 0;
            m_countdownTimer->stop();
            if (m_pushButtonPauseResume)
                m_pushButtonPauseResume->hide();

            ui->progressBarDownload->setValue(0);
            ui->progressBarDownload->setDisabled(true);
            ui->labelEstimatedTimeLeft->setText(QString());
            ui->labelDownloadSize->setText(tr("Interrupted - %1").arg(m_download->interruptReasonString()));
            break;
        }
        case QWebEngineDownloadRequest::DownloadInProgress:
            onDownloadProgress();
            break;
        case QWebEngineDownloadRequest::DownloadCompleted:
            onFinished();
            break;
    }
}

void DownloadItem::openDownloadFolder()
{
    QString folderUrlStr = QString("file://%1").arg(m_downloadDir);
    static_cast<void>(QDesktopServices::openUrl(QUrl(folderUrlStr, QUrl::TolerantMode)));
}

void DownloadItem::updateTimeToCompleteLabel()
{
    if (m_timeRemainingMs < qint64{1000})
        return;

    qint64 numHours = m_timeRemainingMs / 3600000;
    qint64 numMins = (m_timeRemainingMs % 3600000) / 60000;
    qint64 numSeconds = (m_timeRemainingMs % 60000) / 1000;

    QString eta;
    if (numHours > 0)
    {
        eta = tr("%1 hour").arg(numHours);
        if (numHours > 1)
            eta.append('s');

        if (numMins > 1)
            eta.append(tr(", %1 minutes").arg(numMins));
        else if (numMins == 1)
            eta.append(tr(", 1 minute"));
    }
    else if (numMins > 0)
    {
        eta = tr("%1 minute").arg(numMins);
        if (numMins > 1)
            eta.append('s');

        if (numSeconds > 1)
            eta.append(tr(", %1 seconds").arg(numSeconds));
    }
    else if (numSeconds > 0)
    {
        eta = tr("%1 second").arg(numSeconds);
        if (numSeconds > 1)
            eta.append('s');
    }
    eta.append(tr(" remaining"));
    ui->labelEstimatedTimeLeft->setText(eta);
    ui->labelEstimatedTimeLeft->show();

    m_timeRemainingMs -= 1000;
}

void DownloadItem::updateEstimatedTimeLeft(qint64 bytesReceived, qint64 bytesTotal)
{
    qint64 timeElapsedMs = QDateTime::currentMSecsSinceEpoch() - m_startTimeMs;

    // TODO: Account for pause/resume times
    double roughCompletionTime = (static_cast<double>(timeElapsedMs) / static_cast<double>(bytesReceived)) * static_cast<double>(bytesTotal);
    m_timeRemainingMs = static_cast<qint64>(roughCompletionTime) - timeElapsedMs;

    //updateTimeToCompleteLabel();
}
