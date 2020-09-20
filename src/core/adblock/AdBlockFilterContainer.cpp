#include "AdBlockFilterContainer.h"
#include "URL.h"

#include <algorithm>
#include <QHash>

namespace adblock
{

Filter *FilterContainer::findImportantBlockingFilter(
        const QString &baseUrl,
        const QString &requestUrl,
        const QString &requestDomain,
        ElementType typeMask)
{
    for (auto it = m_importantBlockFilters.begin(); it != m_importantBlockFilters.end(); ++it)
    {
        Filter *filter = *it;
        if (filter->isMatch(baseUrl, requestUrl, requestDomain, typeMask))
        {
            it = m_importantBlockFilters.erase(it);
            m_importantBlockFilters.push_front(filter);

            return filter;
        }
    }

    return nullptr;
}

Filter *FilterContainer::findBlockingRequestFilter(
        const QString &requestSecondLevelDomain,
        const QString &baseUrl,
        const QString &requestUrl,
        const QString &requestDomain,
        ElementType typeMask)
{
    Filter *matchingBlockFilter = nullptr;
    auto checkFiltersForMatch = [&](std::deque<Filter*> &filterContainer) {
        for (auto it = filterContainer.begin(); it != filterContainer.end(); ++it)
        {
            Filter *filter = *it;
            if (filter->isMatch(baseUrl, requestUrl, requestDomain, typeMask))
            {
                matchingBlockFilter = filter;
                it = filterContainer.erase(it);
                filterContainer.push_front(filter);
                return;
            }
        }
    };

    auto itr = m_blockFiltersByDomain.find(requestSecondLevelDomain);
    if (itr != m_blockFiltersByDomain.end())
        checkFiltersForMatch(*itr);

    if (matchingBlockFilter == nullptr)
        checkFiltersForMatch(m_blockFilters);

    if (matchingBlockFilter == nullptr)
        checkFiltersForMatch(m_blockFiltersByPattern);

    return matchingBlockFilter;
}

Filter *FilterContainer::findWhitelistingFilter(const QString &baseUrl, const QString &requestUrl, const QString &requestDomain, ElementType typeMask)
{
    for (Filter *filter : m_allowFilters)
    {
        if (filter->isMatch(baseUrl, requestUrl, requestDomain, typeMask))
        {
            return filter;
        }
    }

    return nullptr;
}

bool FilterContainer::hasGenericHideFilter(const QString &requestUrl, const QString &secondLevelDomain) const
{
    for (Filter *filter : m_genericHideFilters)
    {
        if (filter->isMatch(requestUrl, requestUrl, secondLevelDomain, ElementType::Other))
            return true;
    }

    return false;
}

const QString &FilterContainer::getCombinedFilterStylesheet() const
{
    return m_stylesheet;
}

std::vector<Filter*> FilterContainer::getDomainBasedHidingFilters(const QString &domain) const
{
    std::vector<Filter*> result;
    std::vector<Filter*> matches;
    QHash<QString, bool> whitelistedFilters;
    for (Filter *filter : m_domainStyleFilters)
    {
        if (filter->isDomainStyleMatch(domain))
        {
            if (filter->isException())
                whitelistedFilters.insert(filter->getEvalString(), true);
            else
                matches.push_back(filter);
        }
    }
    for (Filter *filter : matches)
    {
        if (whitelistedFilters.constFind(filter->getEvalString()) == whitelistedFilters.constEnd())
            result.push_back(filter);
    }
    return result;
}

std::vector<Filter*> FilterContainer::getDomainBasedCustomHidingFilters(const QString &domain) const
{
    std::vector<Filter*> result;
    for (Filter *filter : m_customStyleFilters)
    {
        if (filter->isDomainStyleMatch(domain))
            result.push_back(filter);
    }
    return result;
}

std::vector<Filter*> FilterContainer::getDomainBasedScriptInjectionFilters(const QString &domain) const
{
    std::vector<Filter*> result;
    for (Filter *filter : m_domainJSFilters)
    {
        if (filter->isDomainStyleMatch(domain))
            result.push_back(filter);
    }
    return result;
}

std::vector<Filter*> FilterContainer::getDomainBasedCosmeticProceduralFilters(const QString &domain) const
{
    std::vector<Filter*> result;
    for (Filter *filter : m_domainProceduralFilters)
    {
        if (filter->isDomainStyleMatch(domain))
            result.push_back(filter);
    }
    return result;
}

std::vector<Filter*> FilterContainer::getMatchingCSPFilters(const QString &requestUrl, const QString &domain) const
{
    std::vector<Filter*> result;
    std::vector<Filter*> matches;
    QHash<QString, bool> whitelistedCSP;
    for (Filter *filter : m_cspFilters)
    {
        if (filter->isMatch(requestUrl, requestUrl, domain, ElementType::CSP))
        {
            if (filter->isException())
                whitelistedCSP.insert(filter->getContentSecurityPolicy(), true);
            else
                matches.push_back(filter);
        }
    }
    for (Filter *filter : matches)
    {
        if (whitelistedCSP.constFind(filter->getContentSecurityPolicy()) == whitelistedCSP.constEnd())
            result.push_back(filter);
    }
    return result;
}

const Filter *FilterContainer::findInlineScriptBlockingFilter(const QString &requestUrl, const QString &domain) const
{
    auto filterCSPCheck = [&](const std::deque<Filter*> &filterContainer) -> const Filter* {
        for (const Filter *filter : filterContainer)
        {
            if (filter->isMatch(requestUrl, requestUrl, domain, ElementType::InlineScript))
            {
                return filter;
            }
        }
        return nullptr;
    };

    const Filter *result = filterCSPCheck(m_importantBlockFilters);

    if (!result)
    {
        auto it = m_blockFiltersByDomain.find(domain);
        if (it != m_blockFiltersByDomain.end())
        {
            result = filterCSPCheck(*it);
        }
    }

    if (!result)
        result = filterCSPCheck(m_blockFilters);

    if (!result)
        result = filterCSPCheck(m_blockFiltersByPattern);

    return result;
}

void FilterContainer::clearFilters()
{
    m_importantBlockFilters.clear();
    m_allowFilters.clear();
    m_blockFilters.clear();
    m_blockFiltersByPattern.clear();
    m_blockFiltersByDomain.clear();
    m_stylesheet.clear();
    m_domainStyleFilters.clear();
    m_domainJSFilters.clear();
    m_domainProceduralFilters.clear();
    m_customStyleFilters.clear();
    m_genericHideFilters.clear();
    m_cspFilters.clear();
}

void FilterContainer::extractFilters(std::vector<Subscription> &subscriptions)
{
    // Used to store css rules for the global stylesheet and domain-specific stylesheets
    QHash<QString, Filter*> stylesheetFilterMap;
    QHash<QString, Filter*> stylesheetExceptionMap;

    // Used to remove bad filters (badfilter option from uBlock)
    QSet<QString> badFilters, badHideFilters;

    // Setup global stylesheet string
    m_stylesheet = QLatin1String("<style>");

    auto isDuplicate = [](const Filter *filter, const std::deque<Filter*> &container) -> bool {
        const QString &filterText = filter->getRule();
        const auto match = std::find_if(std::begin(container), std::end(container), [&filterText](const Filter *f) {
            return filterText.compare(f->getRule()) == 0;
        });
        return match != std::end(container);
    };

    for (Subscription &sub : subscriptions)
    {
        // Add filters to appropriate containers
        const size_t numFilters = sub.getNumFilters();
        for (size_t i = 0; i < numFilters; ++i)
        {
            Filter *filter = sub.getFilter(i);
            if (!filter || filter->getCategory() == FilterCategory::None || filter->getCategory() == FilterCategory::NotImplemented)
                continue;

            if (filter->getCategory() == FilterCategory::Stylesheet)
            {
                if (filter->isException())
                    stylesheetExceptionMap.insert(filter->getEvalString(), filter);
                else
                    stylesheetFilterMap.insert(filter->getEvalString(), filter);
            }
            else if (filter->getCategory() == FilterCategory::StylesheetJS)
            {
                m_domainProceduralFilters.push_back(filter);
            }
            else if (filter->getCategory() == FilterCategory::Scriptlet)
            {
                m_domainJSFilters.push_back(filter);
            }
            else if (filter->getCategory() == FilterCategory::StylesheetCustom)
            {
                m_customStyleFilters.push_back(filter);
            }
            else if (filter->hasElementType(filter->m_blockedTypes, ElementType::BadFilter))
            {
                badFilters.insert(filter->getRule());
            }
            else if (filter->hasElementType(filter->m_blockedTypes, ElementType::CSP))
            {
                if (!filter->hasElementType(filter->m_blockedTypes, ElementType::PopUp)) // Temporary workaround for issues with popup types
                    m_cspFilters.push_back(filter);
            }
            else
            {
                if (filter->isException())
                {
                    if (filter->hasElementType(filter->m_blockedTypes, ElementType::GenericHide))
                        m_genericHideFilters.push_back(filter);
                    else
                        m_allowFilters.push_back(filter);
                }
                else if (filter->isImportant())
                {
                    if (filter->hasElementType(filter->m_blockedTypes, ElementType::GenericHide))
                        badHideFilters.insert(filter->getRule());
                    else
                        m_importantBlockFilters.push_back(filter);
                }
                else if (filter->getCategory() == FilterCategory::StringContains)
                {
                    if (!isDuplicate(filter, m_blockFiltersByPattern))
                        m_blockFiltersByPattern.push_back(filter);
                }
                else if (filter->getCategory() == FilterCategory::Domain)
                {
                    const URL filterUrl { QUrl::fromUserInput(filter->getEvalString()) };
                    const QString filterDomain = filterUrl.getSecondLevelDomain();
                    auto it = m_blockFiltersByDomain.find(filterDomain);
                    if (it != m_blockFiltersByDomain.end())
                    {
                        if (!isDuplicate(filter, it.value()))
                            it->push_back(filter);
                    }
                    else
                    {
                        std::deque<Filter*> queue;
                        queue.push_back(filter);
                        m_blockFiltersByDomain.insert(filterDomain, queue);
                    }
                }
                else if (!isDuplicate(filter, m_blockFilters))
                {
                    m_blockFilters.push_back(filter);
                }
            }
        }
    }

    // Remove bad filters from all applicable filter containers
    auto removeBadFiltersFromVector = [&badFilters](std::vector<Filter*> &filterContainer) {
        for (auto it = filterContainer.begin(); it != filterContainer.end();)
        {
            if (badFilters.contains((*it)->getRule()))
                it = filterContainer.erase(it);
            else
                ++it;
        }
    };
    auto removeBadFiltersFromDeque = [&badFilters](std::deque<Filter*> &filterContainer) {
        for (auto it = filterContainer.begin(); it != filterContainer.end();)
        {
            if (badFilters.contains((*it)->getRule()))
                it = filterContainer.erase(it);
            else
                ++it;
        }
    };

    removeBadFiltersFromVector(m_allowFilters);

    removeBadFiltersFromDeque(m_blockFilters);
    removeBadFiltersFromDeque(m_blockFiltersByPattern);

    for (std::deque<Filter*> &queue : m_blockFiltersByDomain)
    {
        removeBadFiltersFromDeque(queue);
    }

    removeBadFiltersFromVector(m_cspFilters);
    removeBadFiltersFromVector(m_genericHideFilters);

    // Parse stylesheet exceptions
    QHashIterator<QString, Filter*> it(stylesheetExceptionMap);
    while (it.hasNext())
    {
        it.next();

        // Ignore the exception if the blocking rule does not exist
        if (!stylesheetFilterMap.contains(it.key()))
            continue;

        Filter *filter = it.value();
        stylesheetFilterMap.value(it.key())->m_domainWhitelist.unite(filter->m_domainBlacklist);
    }

    // Parse stylesheet blocking rules
    it = QHashIterator<QString, Filter*>(stylesheetFilterMap);
    int numStylesheetRules = 0;
    while (it.hasNext())
    {
        it.next();
        Filter *filter = it.value();

        if (filter->hasDomainRules())
        {
            m_domainStyleFilters.push_back(filter);
            continue;
        }

        if (numStylesheetRules > 1000)
        {
            m_stylesheet = m_stylesheet.left(m_stylesheet.size() - 1);
            m_stylesheet.append(QLatin1String("{ display: none !important; } "));
            numStylesheetRules = 0;
        }

        m_stylesheet.append(filter->getEvalString() + QChar(','));
        ++numStylesheetRules;
    }

    // Complete global stylesheet string
    if (numStylesheetRules > 0)
    {
        m_stylesheet = m_stylesheet.left(m_stylesheet.size() - 1);
        m_stylesheet.append(QLatin1String("{ display: none !important; } "));
    }
    m_stylesheet.append(QLatin1String("</style>"));
}

}
