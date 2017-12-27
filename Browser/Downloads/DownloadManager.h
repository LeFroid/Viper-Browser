#ifndef DOWNLOADMANAGER_H
#define DOWNLOADMANAGER_H

#include <QList>
#include <QNetworkRequest>
#include <QString>
#include <QWidget>

namespace Ui {
class DownloadManager;
}

class DownloadItem;
class DownloadListModel;
class NetworkAccessManager;
class QNetworkReply;

/**
 * @class DownloadManager
 * @brief Handles downloading of content from the web
 */
class DownloadManager : public QWidget
{
    friend class DownloadListModel;

    Q_OBJECT

public:
    /// Constructs the download manager
    explicit DownloadManager(QWidget *parent = 0);

    /// Deallocates interface items and download items
    ~DownloadManager();

    /// Sets the path of the default download directory
    void setDownloadDir(const QString &path);

    /// Returns the path of the download directory
    const QString &getDownloadDir() const;

    /// Sets the network access manager used for downloading content
    void setNetworkAccessManager(NetworkAccessManager *manager);

public slots:
    /// Called when a download request is initiated
    void download(const QNetworkRequest &request, bool askForFileName = true);

    /// Used for internal downloads (not explictly requested by the user)
    DownloadItem *downloadInternal(const QNetworkRequest &request, const QString &downloadDir, bool askForFileName = false, bool writeOverExisting = true);

    /// Handles content that cannot directly be rendered by a web view
    void handleUnsupportedContent(QNetworkReply *reply, bool askForFileName = true);

private:
    /// User interface
    Ui::DownloadManager *ui;

    /// Default download directory
    QString m_downloadDir;

    /// Network access manager
    NetworkAccessManager *m_accessMgr;

    /// List view model
    DownloadListModel *m_model;

protected:
    /// List of downloads
    QList<DownloadItem*> m_downloads;
};

#endif // DOWNLOADMANAGER_H
