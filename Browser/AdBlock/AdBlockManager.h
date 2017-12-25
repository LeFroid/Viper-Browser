#ifndef ADBLOCKMANAGER_H
#define ADBLOCKMANAGER_H

#include "AdBlockSubscription.h"

#include "AdBlocker.h"

#include <QObject>
#include <QString>
#include <vector>

class AdBlockModel;

/**
 * @class AdBlockManager
 * @brief Manages a list of AdBlock Plus style subscriptions
 */
class AdBlockManager : public QObject
{
    friend class AdBlockModel;

    Q_OBJECT

public:
    /// Constructs the AdBlockManager object
    AdBlockManager(QObject *parent = nullptr);

    /// AdBlockManager destructor
    virtual ~AdBlockManager();

    /// Static AdBlockManager instance
    static AdBlockManager &instance();

    /// Returns the model that is used to view and modify ad block subscriptions
    AdBlockModel *getModel();

    /// Returns the base stylesheet for elements to be blocked
    const QString &getStylesheet() const;

    /// Returns the domain-specific blocking stylesheet, or an empty string if not applicable
    QString getDomainStylesheet(const QUrl &url) const;

    /// Returns the domain-specific blocking javascript, or an empty string if not applicable
    QString getDomainJavaScript(const QUrl &url) const;

    /// Determines if the network request should be blocked, returning a BlockedNetworkReply if so, or a
    /// nullptr if the request is allowed
    BlockedNetworkReply *getBlockedReply(const QNetworkRequest &request);

protected:
    /// Returns the number of subscriptions used by the ad block manager
    int getNumSubscriptions() const;

    /// Returns a pointer to the subscription at the given index, or a nullptr if the index is out of range
    const AdBlockSubscription *getSubscription(int index) const;

    /// Toggles the state of the subscription at the given index (enabled <--> disabled)
    void toggleSubscriptionEnabled(int index);

private:
    /// Attempts to determine the type of element being requested, returning the corresponding \ref ElementType
    /// after searching the HTTP headers. Will return ElementType::None if could not be determined or not applicable
    ElementType getElementTypeMask(const QNetworkRequest &request, const QString &requestPath);

    /// Returns the second-level domain string of the given url
    QString getSecondLevelDomain(const QUrl &url) const;

    /// Loads the AdBlock JavaScript template for dynamic filters
    void loadDynamicTemplate();

    /// Loads active subscriptions
    void loadSubscriptions();

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

    /// Ad Block model, used to indirectly view and modify subscriptions in the user interface
    AdBlockModel *m_adBlockModel;
};

#endif // ADBLOCKMANAGER_H
