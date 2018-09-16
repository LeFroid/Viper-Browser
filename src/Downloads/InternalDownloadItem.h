#ifndef InternalDownloadItem_H
#define InternalDownloadItem_H

#include <QFile>
#include <QNetworkReply>
#include <QObject>

/**
 * @class InternalDownloadItem
 * @brief Represents an individual download of some external item
 */
class InternalDownloadItem : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief InternalDownloadItem constructs a new download item
     * @param reply The network reply of the original download request
     * @param downloadDir The default download directory
     * @param askForFileName True if the interface will ask for a specific file name to download the item as, false if default
     * @param writeOverExisting True if the download should overwrite any existing files with naming conflicts, false if else
     * @param parent Pointer to the DownloadManager
     */
    explicit InternalDownloadItem(QNetworkReply *reply, const QString &downloadDir, bool askForFileName, bool writeOverExisting, QObject *parent = nullptr);
    ~InternalDownloadItem();

 signals:
    /// Emitted when the download has successfully completed
    void downloadFinished(const QString &filePath);

private slots:
    /// Called when the download is ready to be read onto the disk
    void onReadyRead();

    /// Called when the download has finished
    void onFinished();

    /// Called if the metadata in the network reply associated with the download has changed
    void onMetaDataChanged();

private:
    /// Connects the interface items to network activity.
    void setupItem();

    /// Sets the icon for the download's file type
    void setIconForItem(const QString &fileName);

    /// Returns the default file name of the download, ensuring there are no naming conflicts on the local file system
    QString getDefaultFileName(const QString &pathWithoutSuffix, const QString &completeSuffix) const;

private:
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

#endif // InternalDownloadItem_H
