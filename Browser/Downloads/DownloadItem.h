#ifndef DOWNLOADITEM_H
#define DOWNLOADITEM_H

#include <QFile>
#include <QNetworkReply>
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
     * @param reply The network reply of the original download request
     * @param downloadDir The default download directory
     * @param askForFileName True if the interface will ask for a specific file name to download the item as, false if default
     * @param writeOverExisting True if the download should overwrite any existing files with naming conflicts, false if else
     * @param parent Pointer to the DownloadManager
     */
    explicit DownloadItem(QNetworkReply *reply, const QString &downloadDir, bool askForFileName, bool writeOverExisting, QWidget *parent = 0);
    ~DownloadItem();

 signals:
    /// Emitted when the download has successfully completed
    void downloadFinished(const QString &filePath);

private slots:
    /// Called when the download is ready to be read onto the disk
    void onReadyRead();

    /// Called when progress has been made in the downloading of this item
    void onDownloadProgress(qint64 bytesReceived, qint64 bytesTotal);

    /// Called when the download has finished
    void onFinished();

    /// Called when a network error has occurred
    void onError(QNetworkReply::NetworkError errorCode);

    /// Called if the metadata in the network reply associated with the download has changed
    void onMetaDataChanged();

private:
    /// Connects the interface items to network activity.
    void setupItem();

    /// Sets the icon for the download's file type
    void setIconForItem(const QString &fileName);

    /// Returns the default file name of the download, ensuring there are no naming conflicts on the local file system
    QString getDefaultFileName(const QString &pathWithoutSuffix, const QString &completeSuffix) const;

    /// Converts the given value in bytes to a user-friendly string. Ex: 1048576 -> 1 MB
    QString getUserByteString(qint64 value) const;

private:
    /// User interface
    Ui::DownloadItem *ui;

    /// Network reply of the original request
    QNetworkReply *m_reply;

    /// Whether or not to ask for a file name to save the download item as
    bool m_askForFileName;

    /// If true, the download will overwrite any existing files with naming conflicts. Otherwise, it will try to resolve the naming conflict
    bool m_writeOverExisting;

    /// Download directory
    QString m_downloadDir;

    /// Total number of bytes received
    qint64 m_bytesReceived;

    /// File being written to on disk
    QFile m_file;

    /// True if download is currently in progress
    bool m_inProgress;

    /// True if download has completed
    bool m_finished;
};

#endif // DOWNLOADITEM_H
