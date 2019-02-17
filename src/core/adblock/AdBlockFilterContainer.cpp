#include "AdBlockFilterContainer.h"
#include "URL.h"

AdBlockFilter *AdBlockFilterContainer::findImportantBlockingFilter(
        const QString &baseUrl,
        const QString &requestUrl,
        const QString &requestDomain,
        ElementType typeMask)
{
    for (auto it = m_importantBlockFilters.begin(); it != m_importantBlockFilters.end(); ++it)
    {
        AdBlockFilter *filter = *it;
        if (filter->isMatch(baseUrl, requestUrl, requestDomain, typeMask))
        {
            it = m_importantBlockFilters.erase(it);
            m_importantBlockFilters.push_front(filter);

            return filter;
        }
    }

    return nullptr;
}

AdBlockFilter *AdBlockFilterContainer::findBlockingRequestFilter(
        const QString &requestSecondLevelDomain,
        const QString &baseUrl,
        const QString &requestUrl,
        const QString &requestDomain,
        ElementType typeMask)
{
    AdBlockFilter *matchingBlockFilter = nullptr;
    auto checkFiltersForMatch = [&](std::deque<AdBlockFilter*> &filterContainer) {
        for (auto it = filterContainer.begin(); it != filterContainer.end(); ++it)
        {
            AdBlockFilter *filter = *it;
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

AdBlockFilter *AdBlockFilterContainer::findWhitelistingFilter(const QString &baseUrl, const QString &requestUrl, const QString &requestDomain, ElementType typeMask)
{
    for (AdBlockFilter *filter : m_allowFilters)
    {
        if (filter->isMatch(baseUrl, requestUrl, requestDomain, typeMask))
        {
            return filter;
        }
    }

    return nullptr;
}

bool AdBlockFilterContainer::hasGenericHideFilter(const QString &requestUrl, const QString &secondLevelDomain) const
{
    for (AdBlockFilter *filter : m_genericHideFilters)
    {
        if (filter->isMatch(requestUrl, requestUrl, secondLevelDomain, ElementType::Other))
            return true;
    }

    return false;
}

const QString &AdBlockFilterContainer::getCombinedFilterStylesheet() const
{
    return m_stylesheet;
}

std::vector<AdBlockFilter*> AdBlockFilterContainer::getDomainBasedHidingFilters(const QString &domain) const
{
    std::vector<AdBlockFilter*> result;
    for (AdBlockFilter *filter : m_domainStyleFilters)
    {
        if (filter->isDomainStyleMatch(domain) && !filter->isException())
            result.push_back(filter);
    }
    return result;
}

std::vector<AdBlockFilter*> AdBlockFilterContainer::getDomainBasedCustomHidingFilters(const QString &domain) const
{
    std::vector<AdBlockFilter*> result;
    for (AdBlockFilter *filter : m_customStyleFilters)
    {
        if (filter->isDomainStyleMatch(domain))
            result.push_back(filter);
    }
    return result;
}

std::vector<AdBlockFilter*> AdBlockFilterContainer::getDomainBasedScriptInjectionFilters(const QString &domain) const
{
    std::vector<AdBlockFilter*> result;
    for (AdBlockFilter *filter : m_domainJSFilters)
    {
        if (filter->isDomainStyleMatch(domain))
            result.push_back(filter);
    }
    return result;
}

std::vector<AdBlockFilter*> AdBlockFilterContainer::getMatchingCSPFilters(const QString &requestUrl, const QString &domain) const
{
    std::vector<AdBlockFilter*> result;
    for (AdBlockFilter *filter : m_cspFilters)
    {
        if (!filter->isException() && filter->isMatch(requestUrl, requestUrl, domain, ElementType::CSP))
        {
            result.push_back(filter);
        }
    }
    return result;
}

const AdBlockFilter *AdBlockFilterContainer::findInlineScriptBlockingFilter(const QString &requestUrl, const QString &domain) const
{
    auto filterCSPCheck = [&](const std::deque<AdBlockFilter*> &filterContainer) -> const AdBlockFilter* {
        for (const AdBlockFilter *filter : filterContainer)
        {
            if (filter->hasElementType(filter->m_blockedTypes, ElementType::InlineScript)
                    && filter->isMatch(requestUrl, requestUrl, domain, ElementType::InlineScript))
            {
                return filter;
            }
        }
        return nullptr;
    };

    const AdBlockFilter *result = filterCSPCheck(m_importantBlockFilters);

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

void AdBlockFilterContainer::clearFilters()
{
    m_importantBlockFilters.clear();
    m_allowFilters.clear();
    m_blockFilters.clear();
    m_blockFiltersByPattern.clear();
    m_blockFiltersByDomain.clear();
    m_stylesheet.clear();
    m_domainStyleFilters.clear();
    m_domainJSFilters.clear();
    m_customStyleFilters.clear();
    m_genericHideFilters.clear();
    m_cspFilters.clear();
}

void AdBlockFilterContainer::extractFilters(std::vector<AdBlockSubscription> &subscriptions)
{
    // Used to store css rules for the global stylesheet and domain-specific stylesheets
    QHash<QString, AdBlockFilter*> stylesheetFilterMap;
    QHash<QString, AdBlockFilter*> stylesheetExceptionMap;

    // Used to remove bad filters (badfilter option from uBlock)
    QSet<QString> badFilters, badHideFilters;

    // Setup global stylesheet string
    m_stylesheet = QLatin1String("<style>");

    for (AdBlockSubscription &sub : subscriptions)
    {
        // Add filters to appropriate containers
        const int numFilters = sub.getNumFilters();
        for (int i = 0; i < numFilters; ++i)
        {
            AdBlockFilter *filter = sub.getFilter(i);
            if (!filter)
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
                    m_blockFiltersByPattern.push_back(filter);
                }
                else if (filter->getCategory() == FilterCategory::Domain)
                {
                    const URL filterUrl { QUrl::fromUserInput(filter->getEvalString()) };
                    const QString filterDomain = filterUrl.getSecondLevelDomain();
                    auto it = m_blockFiltersByDomain.find(filterDomain);
                    if (it != m_blockFiltersByDomain.end())
                    {
                        it->push_back(filter);
                    }
                    else
                    {
                        std::deque<AdBlockFilter*> queue;
                        queue.push_back(filter);
                        m_blockFiltersByDomain.insert(filterDomain, queue);
                    }
                }
                else
                {
                    m_blockFilters.push_back(filter);
                }
            }
        }
    }

    // Remove bad filters from all applicable filter containers
    auto removeBadFiltersFromVector = [&badFilters](std::vector<AdBlockFilter*> &filterContainer) {
        for (auto it = filterContainer.begin(); it != filterContainer.end();)
        {
            if (badFilters.contains((*it)->getRule()))
                it = filterContainer.erase(it);
            else
                ++it;
        }
    };
    auto removeBadFiltersFromDeque = [&badFilters](std::deque<AdBlockFilter*> &filterContainer) {
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

    for (std::deque<AdBlockFilter*> &queue : m_blockFiltersByDomain)
    {
        removeBadFiltersFromDeque(queue);
    }

    removeBadFiltersFromVector(m_cspFilters);
    removeBadFiltersFromVector(m_genericHideFilters);

    // Parse stylesheet exceptions
    QHashIterator<QString, AdBlockFilter*> it(stylesheetExceptionMap);
    while (it.hasNext())
    {
        it.next();

        // Ignore the exception if the blocking rule does not exist
        if (!stylesheetFilterMap.contains(it.key()))
            continue;

        AdBlockFilter *filter = it.value();
        stylesheetFilterMap.value(it.key())->m_domainWhitelist.unite(filter->m_domainBlacklist);
    }

    // Parse stylesheet blocking rules
    it = QHashIterator<QString, AdBlockFilter*>(stylesheetFilterMap);
    int numStylesheetRules = 0;
    while (it.hasNext())
    {
        it.next();
        AdBlockFilter *filter = it.value();

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
