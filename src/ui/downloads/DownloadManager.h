#ifndef DOWNLOADMANAGER_H
#define DOWNLOADMANAGER_H

#include "ISettingsObserver.h"

#include <vector>

#include <QList>
#include <QNetworkRequest>
#include <QString>
#include <QWidget>

namespace Ui {
class DownloadManager;
}

class DownloadItem;
class InternalDownloadItem;
class NetworkAccessManager;
class Settings;

class QNetworkReply;
class QWebEngineDownloadRequest;
class QWebEngineProfile;

/**
 * @class DownloadManager
 * @brief Handles downloading of content from the web
 */
class DownloadManager : public QWidget, public ISettingsObserver
{
    friend class DownloadListModel;

    Q_OBJECT

public:
    /// Constructs the download manager
    /**
     * @brief DownloadManager Constructs the download manager
     * @param settings Pointer to the application settings
     * @param webProfiles Container storing pointers to each active web profile used by the browser (public, private, etc.)
     */
    explicit DownloadManager(Settings *settings, const std::vector<QWebEngineProfile*> &webProfiles);

    /// Deallocates interface items and download items
    ~DownloadManager();

    /// Sets the path of the default download directory
    void setDownloadDir(const QString &path);

    /// Returns the path of the download directory
    const QString &getDownloadDir() const;

    /// Sets the network access manager used for downloading content
    void setNetworkAccessManager(NetworkAccessManager *manager);

public Q_SLOTS:
    /// Clears all downloads from the list
    void clearDownloads();

    /// Called when a download request is initiated
    void onDownloadRequest(QWebEngineDownloadRequest *item);

    /// Used for internal downloads (not explictly requested by the user)
    InternalDownloadItem *downloadInternal(const QNetworkRequest &request, const QString &downloadDir, bool askForFileName = false, bool writeOverExisting = true);

private Q_SLOTS:
    /// Listens for any changes to browser settings that may affect the behavior of the download manager
    void onSettingChanged(BrowserSetting setting, const QVariant &value) override;

private:
    /// User interface
    Ui::DownloadManager *ui;

    /// Default download directory
    QString m_downloadDir;

    /// Network access manager
    NetworkAccessManager *m_accessMgr;

    /// Flag indicating whether or not the user should always be prompted for a download location and filename
    bool m_askWhereToSaveDownloads;

protected:
    /// List of downloads
    QList<DownloadItem*> m_downloads;
};

#endif // DOWNLOADMANAGER_H
