#ifndef ADBLOCKMANAGER_H
#define ADBLOCKMANAGER_H

#include "AdBlockSubscription.h"

#include "AdBlocker.h"

#include <QObject>
#include <QString>
#include <vector>

/**
 * @class AdBlockManager
 * @brief Manages a list of AdBlock Plus style subscriptions
 */
class AdBlockManager : public QObject
{
    Q_OBJECT
public:
    /// Constructs the AdBlockManager object
    AdBlockManager(QObject *parent = nullptr);

    /// AdBlockManager destructor
    virtual ~AdBlockManager();

    /// Static AdBlockManager instance
    static AdBlockManager &instance();

    /// Returns the base stylesheet for elements to be blocked
    const QString &getStylesheet() const;

    /// Returns the domain-specific blocking stylesheet, or an empty string if not applicable
    QString getDomainStylesheet(const QString &domain) const;

    /// Determines if the network request should be blocked, returning a BlockedNetworkReply if so, or a
    /// nullptr if the request is allowed
    BlockedNetworkReply *getBlockedReply(const QNetworkRequest &request);

    /// Returns the second-level domain string of the given url
    QString getSecondLevelDomain(const QUrl &url);

private:
    /// Attempts to determine the type of element being requested, returning the corresponding \ref ElementType
    /// after searching the HTTP headers. Will return ElementType::None if could not be determined or not applicable
    ElementType getElementTypeMask(const QNetworkRequest &request, const QString &requestPath);

    /// Loads active subscriptions
    void loadSubscriptions();

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

    /// Container of content blocking subscriptions
    std::vector<AdBlockSubscription> m_subscriptions;

    /// Container of filters that block content
    std::vector<AdBlockFilter*> m_blockFilters;

    /// Container of filters that whitelist content
    std::vector<AdBlockFilter*> m_allowFilters;

    /// Container of filters that have domain-specific stylesheet rules
    std::vector<AdBlockFilter*> m_domainStyleFilters;
};

#endif // ADBLOCKMANAGER_H
