#ifndef FAVICONMANAGER_H
#define FAVICONMANAGER_H

#include "DatabaseTaskScheduler.h"
#include "DatabaseWorker.h"
#include "FaviconStore.h"
#include "FaviconTypes.h"
#include "LRUCache.h"

#include <unordered_map>
#include <memory>
#include <mutex>

#include <QHash>
#include <QIcon>
#include <QObject>
#include <QString>
#include <QUrl>

class NetworkAccessManager;
class QNetworkReply;

/**
 * @class FaviconManager
 * @brief Acts as an interface between the \ref FaviconStore and the components
 *        of the web browser that require a favicon for any given URL
 */
class FaviconManager : public QObject
{
    Q_OBJECT

public:
    /// Constructs the favicon manager
    explicit FaviconManager(const QString &databaseFile);

    /// Passes the instance of the network access manager, so the favicon manager can download
    /// new icons as they are referenced by a web page.
    void setNetworkAccessManager(NetworkAccessManager *networkAccessManager);

    /// Searches for a favicon associated with the given URL, returning either the favicon
    /// or an empty favicon if it could not be found
    QIcon getFavicon(const QUrl &url);

    /**
     * @brief Attempts to update favicon for a specific URL in the database.
     * @param iconUrl The location in which the favicon is stored.
     * @param pageUrl The URL of the page displaying the favicon.
     * @param pageIcon The favicon on the page.
     */
    void updateIcon(const QUrl &iconUrl, const QUrl &pageUrl, const QIcon &pageIcon);

private Q_SLOTS:
    /// Called after the request for a favicon has been completed
    void onReplyFinished(QNetworkReply *reply);

private:
    /// Returns the given URL in string form
    QString getUrlAsString(const QUrl &url) const;

private:
    /// Favicon data store
    std::unique_ptr<FaviconStore> m_faviconStore;

    /// Used to download icons when a new one is referenced
    NetworkAccessManager *m_networkAccessManager;

    /// Mapping of favicon IDs (as stored in \ref FaviconStore ) to their corresponding QIcons
    std::unordered_map<int, QIcon> m_iconMap;

    /// Cache of most recently visited URLs and the icons associated with those pages
    LRUCache<std::string, QIcon> m_iconCache;

    /// Used when updating the LRU cache for thread safety
    mutable std::mutex m_mutex;
};

#endif // FAVICONMANAGER_H
