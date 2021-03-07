#ifndef ADBLOCKMANAGER_H
#define ADBLOCKMANAGER_H

#include "AdBlockFilter.h"
#include "AdBlockFilterContainer.h"
#include "AdBlockSubscription.h"
#include "LRUCache.h"
#include "ServiceLocator.h"
#include "Settings.h"
#include "ISettingsObserver.h"
#include "URL.h"

#include <QHash>
#include <QObject>
#include <QString>
#include <QWebEngineUrlRequestInfo>

#include <deque>
#include <vector>

class BrowserApplication;
class DownloadManager;

namespace adblock
{
    class AdBlockLog;
    class AdBlockModel;
    class RequestHandler;

/**
 * @defgroup AdBlock Advertisement Blocking System
 * An implementation of the AdBlockPlus and uBlock Origin style content filtering system
 */

/**
 * @class AdBlockManager
 * @ingroup AdBlock
 * @brief Manages a list of AdBlock Plus style subscriptions, and compares
 *        network requests against blocking and allowing filter rules before
 *        letting any requests go through.
 */
class AdBlockManager : public QObject, public ISettingsObserver
{
    friend class FilterParser;
    friend class AdBlockModel;
    friend class ::BrowserApplication;

    Q_OBJECT

public:
    /// Constructs the AdBlockManager
    AdBlockManager(const ViperServiceLocator &serviceLocator, QObject *parent = nullptr);

    /// AdBlockManager destructor
    ~AdBlockManager();

    /// Sets the state of the ad block manager. If true, it will filter network requests as per filter rules. Otherwise, no blocking will be done
    void setEnabled(bool value);

    /// Returns the logging instance
    AdBlockLog *getLog() const;

    /// Returns the model that is used to view and modify ad block subscriptions
    AdBlockModel *getModel();

    /// Returns the base stylesheet for elements to be blocked. If the given url matches a generichide filter, this will return an empty string
    const QString &getStylesheet(const URL &url) const;

    /// Returns the domain-specific blocking stylesheet, or an empty string if not applicable
    const QString &getDomainStylesheet(const URL &url);

    /// Returns the domain-specific blocking javascript, or an empty string if not applicable
    const QString &getDomainJavaScript(const URL &url);

    /// Returns true if the given request should be blocked, false if else
    bool shouldBlockRequest(QWebEngineUrlRequestInfo &info, const QUrl &firstPartyUrl);

    /// Returns the total number of network requests that have been blocked by the ad blocking system
    quint64 getRequestsBlockedCount() const;

    /// Returns the number of ads that were blocked on the page with the given URL during its last page load
    int getNumberAdsBlocked(const QUrl &url) const;

    /// Searches for and returns the value from the resource map that is associated with the given key. Returns an empty string if not found
    QString getResource(const QString &key) const;

    /// Returns the content type of the resource with the given key. Returns an empty string if the key is not found
    QString getResourceContentType(const QString &key) const;

public Q_SLOTS:
    /// Attempt to update ad block subscriptions
    void updateSubscriptions();

    /**
     * @brief Attempts to download and install the uBlock-style resource file from the given URL
     * @param url The location of the resource file to be installed
     */
    void installResource(const QUrl &url);

    /**
     * @brief Attempts to download and install the subscription from the given URL
     * @param url The location of the subscription file to be installed
     */
    void installSubscription(const QUrl &url);

    /// Creates and registers new \ref Subscription to be associated with user-set filter rules
    void createUserSubscription();

    /// Informs the AdBlockManager to begin keeping track of the number of ads that were blocked on the page with the given url
    void loadStarted(const QUrl &url);

    /// Reloads the filter list subscriptions
    void reloadSubscriptions();

// Called by AdBlockModel:
protected:
    /// Returns the number of subscriptions used by the ad block manager
    int getNumSubscriptions() const;

    /// Returns a pointer to the subscription at the given index, or a nullptr if the index is out of range
    const Subscription *getSubscription(int index) const;

    /// Toggles the state of the subscription at the given index (enabled <--> disabled)
    void toggleSubscriptionEnabled(int index);

    /// Removes the subscription at the given index, if the index is within range, and reloads existing subscriptions
    void removeSubscription(int index);

// Called by BrowserApplication:
protected:
    /// Loads active subscriptions
    void loadSubscriptions();

private Q_SLOTS:
    /// Loads the uBlock Origin-style resource file into the resource map
    void loadResourceFile(const QString &path);

    /// Listens for any settings changes that affect the advertisement blocking system (ex: enable/disable ad block)
    void onSettingChanged(BrowserSetting setting, const QVariant &value) override;

private:
    /// Returns the proper resource name, given an alias (ex: acis -> abort-current-inline-script.js)
    /// Returns an empty string if no mapping is found
    QString getResourceFromAlias(const QString &alias) const;

    /// Loads the AdBlock JavaScript template for dynamic filters
    void loadDynamicTemplate();

    /// Load uBlock Origin-style resources file(s) from m_subscriptionDir/resources folder
    void loadUBOResources();

    /// Load built-in resource aliases for filter compatibility
    void loadResourceAliases();

    /// Clears current filter data
    void clearFilters();

    /// Extracts pointers to filters from subscriptions into the appropriate containers
    void extractFilters();

    /// Saves subscription information to disk, called by destructor
    void save();

private:
    /// Stores the union of all subscription list filters
    FilterContainer m_filterContainer;

    /// Download manager, required to update subscription lists
    DownloadManager *m_downloadManager;

    /// True if AdBlock is enabled, false if disabled
    bool m_enabled;

    /// JSON configuration file path
    QString m_configFile;

    /// Directory path in which subscriptions are located
    QString m_subscriptionDir;

    /// JavaScript template for uBlock style cosmetic filters
    QString m_cosmeticJSTemplate;

    /// Container of content blocking subscriptions
    std::vector<Subscription> m_subscriptions;

    /// Mapping of short names of resources to their full names
    QHash<QString, QString> m_resourceAliasMap;

    /// Resources available to filters by referencing the key. Available for redirect options as well as script injections
    QHash<QString, QString> m_resourceMap;

    /// Mapping of resource names, from the resource map, to their respective content types
    QHash<QString, QString> m_resourceContentTypeMap;

    /// A cache of the most recently used domain-specific stylesheets
    LRUCache<std::string, QString> m_domainStylesheetCache;

    /// A cache of the most recently used javascript injection scripts for specific URLs
    LRUCache<std::string, QString> m_jsInjectionCache;

    /// Empty string, used when getDomainStylesheet returns nothing
    QString m_emptyStr;

    /// Ad Block model, used to indirectly view and modify subscriptions in the user interface
    AdBlockModel *m_adBlockModel;

    /// Stores logs associated with actions taken by the ad block system
    AdBlockLog *m_log;

    /// Performs network request matching to filters, and keeps count of the number of blocked requests (total + per URL)
    RequestHandler *m_requestHandler;
};

}

#endif // ADBLOCKMANAGER_H
