#ifndef ADBLOCKFILTERCONTAINER_H
#define ADBLOCKFILTERCONTAINER_H

#include "AdBlockFilter.h"
#include "AdBlockSubscription.h"

#include <deque>
#include <functional>
#include <vector>

#include <QHash>
#include <QObject>
#include <QString>

namespace adblock
{

/**
 * @class FilterContainer
 * @brief Stores filter rules in various containers, optimized for fastest lookup time.
 * @ingroup AdBlock
 */
class FilterContainer
{
    friend class AdBlockManager;

public:
    /// Default constructor
    FilterContainer() = default;

    /**
     * @brief Searches the important blocking filter container for the first match
     * @param baseUrl URL of the original network request
     * @param requestUrl URL of the actual network request
     * @param requestDomain Domain of the request URL
     * @param typeMask Element type(s) associated with the request.
     * @return A pointer to the first matching filter rule, or a nullptr if not found
     */
    Filter *findImportantBlockingFilter(const QString &baseUrl, const QString &requestUrl, const QString &requestDomain, ElementType typeMask);

    /**
     * @brief Searches the blocking filter containers (excluding the important blocking filter container) for the first network request match
     * @param requestSecondLevelDomain Second level domain of the requested URL
     * @param baseUrl URL of the original network request
     * @param requestUrl URL of the actual network request
     * @param requestDomain Domain of the request URL
     * @param typeMask Element type(s) associated with the request.
     * @return A pointer to the first matching filter rule, or a nullptr if not found
     */
    Filter *findBlockingRequestFilter(const QString &requestSecondLevelDomain, const QString &baseUrl,
                                             const QString &requestUrl, const QString &requestDomain, ElementType typeMask);

    /**
     * @brief Searches the whitelisting filter container for the first match
     * @param baseUrl URL of the original network request
     * @param requestUrl URL of the actual network request
     * @param requestDomain Domain of the request URL
     * @param typeMask Element type(s) associated with the request.
     * @return A pointer to the first matching filter rule, or a nullptr if not found
     */
    Filter *findWhitelistingFilter(const QString &baseUrl, const QString &requestUrl, const QString &requestDomain, ElementType typeMask);

    /// Searches for a matching domain-specific filters of which the generic element hiding rules do not apply.
    /// Returns true if a matching filter was found, or false otherwise.
    bool hasGenericHideFilter(const QString &requestUrl, const QString &secondLevelDomain) const;

    /// Returns the union of all global CSS hiding rules, in the form of an HTML <style>...</style> node
    const QString &getCombinedFilterStylesheet() const;

    /// Returns a vector containing any filters that are meant to hide elements on the given domain
    std::vector<Filter*> getDomainBasedHidingFilters(const QString &domain) const;

    /// Returns a vector containing any filters that have specific CSS rules to be applied on the given domain
    std::vector<Filter*> getDomainBasedCustomHidingFilters(const QString &domain) const;

    /// Returns a vector containing any filters that have specific javascript code to be injected on the given domain
    std::vector<Filter*> getDomainBasedScriptInjectionFilters(const QString &domain) const;

    /// Returns a vector containing any filters that have use a dynamic method of hiding one or more elements on the given domain
    std::vector<Filter*> getDomainBasedCosmeticProceduralFilters(const QString &domain) const;

    /// Returns a vector containing any filters that have a CSP rule to be applied to the given request
    std::vector<Filter*> getMatchingCSPFilters(const QString &requestUrl, const QString &domain) const;

    /**
     * @brief Searches for a filter rule that prevents the given page from loading inline scripts
     * @param requestUrl Full requested URL, in string form
     * @param domain Domain of the requested URL
     * @return A pointer to a matching filter if found, or a nullptr otherwise
     */
    const Filter *findInlineScriptBlockingFilter(const QString &requestUrl, const QString &domain) const;

protected:
    /// Clears current filter data
    void clearFilters();

    /// Extracts ad blocking filter rules from the given container of filter list subscriptions.
    void extractFilters(std::vector<Subscription> &subscriptions);

private:
    /// Global adblock stylesheet
    QString m_stylesheet;

    /// Container of important blocking filters that are checked before allow filters on network requests
    std::deque<Filter*> m_importantBlockFilters;

    /// Container of filters that block content
    std::deque<Filter*> m_blockFilters;

    /// Container of filters that block content based on a partial string match (needle in haystack)
    std::deque<Filter*> m_blockFiltersByPattern;

    /// Hashmap of filters that are of the Domain category (||some.domain.com^ style filter rules)
    QHash<QString, std::deque<Filter*>> m_blockFiltersByDomain;

    /// Container of filters that whitelist content
    std::vector<Filter*> m_allowFilters;

    /// Container of filters that have domain-specific stylesheet rules
    std::vector<Filter*> m_domainStyleFilters;

    /// Container of filters that have domain-specific javascript rules
    std::vector<Filter*> m_domainJSFilters;

    /// Container of filters that have domain-specific procedural filter rules
    std::vector<Filter*> m_domainProceduralFilters;

    /// Container of filters that have custom stylesheet values (:style filter option)
    std::vector<Filter*> m_customStyleFilters;

    /// Container of domain-specific filters for which the generic element hiding rules do not apply
    std::vector<Filter*> m_genericHideFilters;

    /// Container of filters that set the content security policy for a matching domain
    std::vector<Filter*> m_cspFilters;
};

}

#endif // ADBLOCKFILTERCONTAINER_H
