#ifndef DOWNLOADITEM_H
#define DOWNLOADITEM_H

#include <QFile>
#include <QNetworkReply>
#include <QWebEngineDownloadItem>
#include <QWidget>

namespace Ui {
class DownloadItem;
}

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
    explicit DownloadItem(QWebEngineDownloadItem *item, QWidget *parent = nullptr);
    ~DownloadItem();

signals:
    /// Emitted when the user requests the download item to be removed from the visible downloads list
    void removeFromList();

protected:
    /// Event handler for context menu requests
    void contextMenuEvent(QContextMenuEvent *event) override;

private slots:
    /// Called when progress has been made in the downloading of this item
    void onDownloadProgress(qint64 bytesReceived, qint64 bytesTotal);

    /// Called when the download has finished
    void onFinished();

    /// Called when the state of the download has changed
    void onStateChanged(QWebEngineDownloadItem::DownloadState state);

    /// Opens the folder containing the downloaded item
    void openDownloadFolder();

private:
    /// Connects the interface items to network activity.
    void setupItem();

    /// Sets the icon for the download's file type
    void setIconForItem(const QString &fileName);

    /// Converts the given value in bytes to a user-friendly string. Ex: 1048576 -> 1 MB
    QString getUserByteString(qint64 value) const;

private:
    /// User interface
    Ui::DownloadItem *ui;

    /// Pointer to the download item
    QWebEngineDownloadItem *m_download;

    /// Download directory
    QString m_downloadDir;

    /// Total number of bytes received
    qint64 m_bytesReceived;

    /// True if download is currently in progress
    bool m_inProgress;
};

#endif // DOWNLOADITEM_H
