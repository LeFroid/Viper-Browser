#ifndef ADBLOCKMANAGER_H
#define ADBLOCKMANAGER_H

#include "AdBlockSubscription.h"
#include "BlockedNetworkReply.h"
#include "LRUCache.h"

#include <QHash>
#include <QObject>
#include <QString>
#include <QWebEngineUrlRequestInfo>
#include <vector>

class AdBlockModel;

/**
 * @class AdBlockManager
 * @brief Manages a list of AdBlock Plus style subscriptions
 */
class AdBlockManager : public QObject
{
    friend class AdBlockFilterParser;
    friend class AdBlockModel;
    friend class AdBlockWidget;
    friend class BrowserApplication;

    Q_OBJECT

public:
    /// Constructs the AdBlockManager object
    AdBlockManager(QObject *parent = nullptr);

    /// AdBlockManager destructor
    virtual ~AdBlockManager();

    /// Static AdBlockManager instance
    static AdBlockManager &instance();

    /// Sets the state of the ad block manager. If true, it will filter network requests as per filter rules. Otherwise, no blocking will be done
    void setEnabled(bool value);

    /// Returns the model that is used to view and modify ad block subscriptions
    AdBlockModel *getModel();

    /// Returns the base stylesheet for elements to be blocked. If the given url matches a generichide filter, this will return an empty string
    const QString &getStylesheet(const QUrl &url) const;

    /// Returns the domain-specific blocking stylesheet, or an empty string if not applicable
    const QString &getDomainStylesheet(const QUrl &url);

    /// Returns the domain-specific blocking javascript, or an empty string if not applicable
    QString getDomainJavaScript(const QUrl &url) const;

    /// Returns true if the given request should be blocked, false if else
    bool shouldBlockRequest(const QWebEngineUrlRequestInfo &info);

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

// Called by AdBlockFilterParser:
protected:
    /// Searches for and returns the value from the resource map that is associated with the given key. Returns an empty string if not found
    QString getResource(const QString &key) const;

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
    std::vector<AdBlockFilter*> m_importantBlockFilters;

    /// Container of filters that block content
    std::vector<AdBlockFilter*> m_blockFilters;

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

    /// Resources available to filters by referencing the key. Available for redirect options as well as script injections
    QHash<QString, QString> m_resourceMap;

    /// A cache of the most recently used domain-specific stylesheets
    LRUCache<std::string, QString> m_domainStylesheetCache;

    /// Empty string, used when getDomainStylesheet returns nothing
    QString m_emptyStr;

    /// Ad Block model, used to indirectly view and modify subscriptions in the user interface
    AdBlockModel *m_adBlockModel;
};

#endif // ADBLOCKMANAGER_H
