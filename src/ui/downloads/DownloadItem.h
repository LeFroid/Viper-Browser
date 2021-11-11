#ifndef DOWNLOADITEM_H
#define DOWNLOADITEM_H

#include <QFile>
#include <QNetworkReply>
#include <QWebEngineDownloadRequest>
#include <QWidget>

namespace Ui {
class DownloadItem;
}

class QPushButton;
class QTimer;

/**
 * @class DownloadItem
 * @brief Represents an individual download of some external item
 */
class DownloadItem : public QWidget
{
    Q_OBJECT

public:
    /**
     * @brief DownloadItem constructs a new download item
     * @param item Pointer to the web engine download item
     * @param writeOverExisting True if the download should overwrite any existing files with naming conflicts, false if else
     * @param parent Pointer to the DownloadManager
     */
    explicit DownloadItem(QWebEngineDownloadRequest *item, QWidget *parent = nullptr);

    /// Destructor
    ~DownloadItem();

    /// Returns true if this item is finished downloading (either successful download, canceled, interrupted,
    /// etc), false otherwise
    bool isFinished() const;

public Q_SLOTS:
    /// Cancels the download
    void cancel();

Q_SIGNALS:
    /// Emitted when the user requests the download item to be removed from the visible downloads list
    void removeFromList();

protected:
    /// Event handler for context menu requests
    void contextMenuEvent(QContextMenuEvent *event) override;

private Q_SLOTS:
    /// Called when progress has been made in the downloading of this item
    void onDownloadProgress();

    /// Called when the download has finished
    void onFinished();

    /// Called when the state of the download has changed
    void onStateChanged(QWebEngineDownloadRequest::DownloadState state);

    /// Opens the folder containing the downloaded item
    void openDownloadFolder();

    /// Updates the ETA label to the most recent estimate
    void updateTimeToCompleteLabel();

private:
    /// Connects the interface items to network activity.
    void setupItem();

    /// Sets the icon for the download's file type
    void setIconForItem(const QString &fileName);

    /// Updates the ETA label, based on current progress
    void updateEstimatedTimeLeft(qint64 bytesReceived, qint64 bytesTotal);

private:
    /// User interface
    Ui::DownloadItem *ui;

    /// Pointer to the download item
    QWebEngineDownloadRequest *m_download;

    /// Download directory
    QString m_downloadDir;

    /// Total number of bytes received
    qint64 m_bytesReceived;

    /// Button used to pause or resume the download (only enabled if QtWebEngine version is >= 5.10)
    QPushButton *m_pushButtonPauseResume;

    /// The time when this download was initiated (milliseconds)
    qint64 m_startTimeMs;

    /// Time of last update to the ETA
    qint64 m_lastUpdateMs;

    /// ETA until download is complete, in milliseconds
    qint64 m_timeRemainingMs;

    /// ETA countdown timer, to update the label, making a smoother progression
    QTimer *m_countdownTimer;
};

#endif // DOWNLOADITEM_H
