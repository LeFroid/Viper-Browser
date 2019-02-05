#ifndef ADBLOCKMANAGER_H
#define ADBLOCKMANAGER_H

#include "AdBlockFilter.h"
#include "AdBlockSubscription.h"
#include "LRUCache.h"
#include "ServiceLocator.h"
#include "URL.h"

#include <QHash>
#include <QObject>
#include <QString>
#include <QWebEngineUrlRequestInfo>

#include <deque>
#include <vector>

class AdBlockLog;
class AdBlockModel;
class DownloadManager;
class Settings;

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
class AdBlockManager : public QObject
{
    friend class AdBlockFilterParser;
    friend class AdBlockModel;
    friend class AdBlockWidget;
    friend class BrowserApplication;
    friend class BlockedSchemeHandler;

    Q_OBJECT

public:
    /// Constructs the AdBlockManager
    AdBlockManager(ViperServiceLocator &serviceLocator, Settings *settings, QObject *parent = nullptr);

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
    bool shouldBlockRequest(QWebEngineUrlRequestInfo &info);

    /// Returns the total number of network requests that have been blocked by the ad blocking system
    quint64 getRequestsBlockedCount() const;

    /// Returns the number of ads that were blocked on the page with the given URL during its last page load
    int getNumberAdsBlocked(const QUrl &url);

public slots:
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

    /// Creates and registers new \ref AdBlockSubscription to be associated with user-set filter rules
    void createUserSubscription();

    /// Informs the AdBlockManager to begin keeping track of the number of ads that were blocked on the page with the given url
    void loadStarted(const QUrl &url);

// Called by AdBlockFilterParser and BlockedSchemeHandler:
protected:
    /// Searches for and returns the value from the resource map that is associated with the given key. Returns an empty string if not found
    QString getResource(const QString &key) const;

    /// Returns the content type of the resource with the given key. Returns an empty string if the key is not found
    QString getResourceContentType(const QString &key) const;

// Called by AdBlockModel:
protected:
    /// Returns the number of subscriptions used by the ad block manager
    int getNumSubscriptions() const;

    /// Returns a pointer to the subscription at the given index, or a nullptr if the index is out of range
    const AdBlockSubscription *getSubscription(int index) const;

    /// Toggles the state of the subscription at the given index (enabled <--> disabled)
    void toggleSubscriptionEnabled(int index);

    /// Removes the subscription at the given index, if the index is within range, and reloads existing subscriptions
    void removeSubscription(int index);

// Called by AdBlockWidget:
protected slots:
    /// Reloads the ad blocking subscriptions
    void reloadSubscriptions();

// Called by BrowserApplication:
protected:
    /// Loads active subscriptions
    void loadSubscriptions();

private slots:
    /// Loads the uBlock Origin-style resource file into the resource map
    void loadResourceFile(const QString &path);

private:
    /// Returns true if the request should not be processed by the Ad Block system based on its scheme
    bool isSchemeWhitelisted(const QString &scheme) const;

    /// Returns the \ref ElementType of the network request, which is used to check for filter option/type matches
    ElementType getRequestType(const QWebEngineUrlRequestInfo &info) const;

    /// Returns the second-level domain string of the given url
    QString getSecondLevelDomain(const QUrl &url) const;

    /// Loads the AdBlock JavaScript template for dynamic filters
    void loadDynamicTemplate();

    /// Load uBlock Origin-style resources file(s) from m_subscriptionDir/resources folder
    void loadUBOResources();

    /// Clears current filter data
    void clearFilters();

    /// Extracts pointers to filters from subscriptions into the appropriate containers
    void extractFilters();

    /// Saves subscription information to disk, called by destructor
    void save();

private:
    /// Download manager, required to update subscription lists
    DownloadManager *m_downloadManager;

    /// True if AdBlock is enabled, false if disabled
    bool m_enabled;

    /// JSON configuration file path
    QString m_configFile;

    /// Directory path in which subscriptions are located
    QString m_subscriptionDir;

    /// Global adblock stylesheet
    QString m_stylesheet;

    /// JavaScript template for uBlock style cosmetic filters
    QString m_cosmeticJSTemplate;

    /// Container of content blocking subscriptions
    std::vector<AdBlockSubscription> m_subscriptions;

    /// Container of important blocking filters that are checked before allow filters on network requests
    std::deque<AdBlockFilter*> m_importantBlockFilters;

    /// Container of filters that block content
    std::deque<AdBlockFilter*> m_blockFilters;

    /// Container of filters that block content based on a partial string match (needle in haystack)
    std::deque<AdBlockFilter*> m_blockFiltersByPattern;

    /// Hashmap of filters that are of the Domain category (||some.domain.com^ style filter rules)
    QHash<QString, std::deque<AdBlockFilter*>> m_blockFiltersByDomain;

    /// Container of filters that whitelist content
    std::vector<AdBlockFilter*> m_allowFilters;

    /// Container of filters that have domain-specific stylesheet rules
    std::vector<AdBlockFilter*> m_domainStyleFilters;

    /// Container of filters that have domain-specific javascript rules
    std::vector<AdBlockFilter*> m_domainJSFilters;

    /// Container of filters that have custom stylesheet values (:style filter option)
    std::vector<AdBlockFilter*> m_customStyleFilters;

    /// Container of domain-specific filters for which the generic element hiding rules (in m_stylesheet) do not apply
    std::vector<AdBlockFilter*> m_genericHideFilters;

    /// Container of filters that set the content security policy for a matching domain
    std::vector<AdBlockFilter*> m_cspFilters;

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

    /// Stores the number of network requests that have been blocked by the ad block system
    quint64 m_numRequestsBlocked;

    /// Hash map of URLs to the number of requests that were blocked on that given URL
    QHash<QUrl, int> m_pageAdBlockCount;

    /// Stores logs associated with actions taken by the ad block system
    AdBlockLog *m_log;
};

#endif // ADBLOCKMANAGER_H
