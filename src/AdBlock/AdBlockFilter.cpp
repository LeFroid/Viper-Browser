#include <algorithm>
#include <array>
#include "AdBlockFilter.h"
#include "Bitfield.h"
#include "FastHash.h"
#include "URL.h"

AdBlockFilter::AdBlockFilter(const QString &rule) :
    m_category(FilterCategory::None),
    m_ruleString(rule),
    m_evalString(),
    m_contentSecurityPolicy(),
    m_exception(false),
    m_important(false),
    m_disabled(false),
    m_redirect(false),
    m_redirectName(),
    m_allowedTypes(ElementType::None),
    m_blockedTypes(ElementType::None),
    m_matchCase(false),
    m_matchAll(false),
    m_domainBlacklist(),
    m_domainWhitelist(),
    m_regExp(nullptr),
    m_differenceHash(0),
    m_evalStringHash(0),
    m_needleWStr()
{
}

AdBlockFilter::AdBlockFilter(const AdBlockFilter &other) :
    m_category(other.m_category),
    m_ruleString(other.m_ruleString),
    m_evalString(other.m_evalString),
    m_contentSecurityPolicy(other.m_contentSecurityPolicy),
    m_exception(other.m_exception),
    m_important(other.m_important),
    m_disabled(other.m_disabled),
    m_redirect(other.m_redirect),
    m_redirectName(other.m_redirectName),
    m_allowedTypes(other.m_allowedTypes),
    m_blockedTypes(other.m_blockedTypes),
    m_matchCase(other.m_matchCase),
    m_matchAll(other.m_matchAll),
    m_domainBlacklist(other.m_domainBlacklist),
    m_domainWhitelist(other.m_domainWhitelist),
    m_regExp(other.m_regExp ? std::make_unique<QRegularExpression>(*other.m_regExp) : nullptr),
    m_differenceHash(other.m_differenceHash),
    m_evalStringHash(other.m_evalStringHash),
    m_needleWStr(other.m_needleWStr)
{
}

AdBlockFilter::AdBlockFilter(AdBlockFilter &&other) :
    m_category(other.m_category),
    m_ruleString(other.m_ruleString),
    m_evalString(other.m_evalString),
    m_contentSecurityPolicy(other.m_contentSecurityPolicy),
    m_exception(other.m_exception),
    m_important(other.m_important),
    m_disabled(other.m_disabled),
    m_redirect(other.m_redirect),
    m_redirectName(other.m_redirectName),
    m_allowedTypes(other.m_allowedTypes),
    m_blockedTypes(other.m_blockedTypes),
    m_matchCase(other.m_matchCase),
    m_matchAll(other.m_matchAll),
    m_domainBlacklist(std::move(other.m_domainBlacklist)),
    m_domainWhitelist(std::move(other.m_domainWhitelist)),
    m_regExp(std::move(other.m_regExp)),
    m_differenceHash(other.m_differenceHash),
    m_evalStringHash(other.m_evalStringHash),
    m_needleWStr(other.m_needleWStr)
{
}

AdBlockFilter &AdBlockFilter::operator =(const AdBlockFilter &other)
{
    if(this != &other)
    {
        m_category = other.m_category;
        m_ruleString = other.m_ruleString;
        m_evalString = other.m_evalString;
        m_contentSecurityPolicy = other.m_contentSecurityPolicy;
        m_exception = other.m_exception;
        m_important = other.m_important;
        m_disabled = other.m_disabled;
        m_redirect = other.m_redirect;
        m_redirectName = other.m_redirectName;
        m_allowedTypes = other.m_allowedTypes;
        m_blockedTypes = other.m_blockedTypes;
        m_matchCase = other.m_matchCase;
        m_matchAll = other.m_matchAll;
        m_domainBlacklist = other.m_domainBlacklist;
        m_domainWhitelist = other.m_domainWhitelist;
        m_regExp = (other.m_regExp ? std::make_unique<QRegularExpression>(*other.m_regExp) : nullptr);
        m_differenceHash = other.m_differenceHash;
        m_evalStringHash = other.m_evalStringHash;
        m_needleWStr = other.m_needleWStr;
    }

    return *this;
}

AdBlockFilter &AdBlockFilter::operator =(AdBlockFilter &&other)
{
    if(this != &other)
    {
        m_category = other.m_category;
        m_ruleString = other.m_ruleString;
        m_evalString = other.m_evalString;
        m_contentSecurityPolicy = other.m_contentSecurityPolicy;
        m_exception = other.m_exception;
        m_important = other.m_important;
        m_disabled = other.m_disabled;
        m_redirect = other.m_redirect;
        m_redirectName = other.m_redirectName;
        m_allowedTypes = other.m_allowedTypes;
        m_blockedTypes = other.m_blockedTypes;
        m_matchCase = other.m_matchCase;
        m_matchAll = other.m_matchAll;
        m_domainBlacklist = std::move(other.m_domainBlacklist);
        m_domainWhitelist = std::move(other.m_domainWhitelist);
        m_regExp = std::move(other.m_regExp);
        m_differenceHash = other.m_differenceHash;
        m_evalStringHash = other.m_evalStringHash;
        m_needleWStr = other.m_needleWStr;
    }
    return *this;
}

AdBlockFilter::~AdBlockFilter()
{
}

FilterCategory AdBlockFilter::getCategory() const
{
    return m_category;
}

void AdBlockFilter::setRule(const QString &rule)
{
    m_ruleString = rule;
}

const QString &AdBlockFilter::getRule() const
{
    return m_ruleString;
}

const QString &AdBlockFilter::getEvalString() const
{
    return m_evalString;
}

const QString &AdBlockFilter::getContentSecurityPolicy() const
{
    return m_contentSecurityPolicy;
}

bool AdBlockFilter::isException() const
{
    return m_exception;
}

bool AdBlockFilter::isImportant() const
{
    return m_important;
}

bool AdBlockFilter::hasDomainRules() const
{
    return !m_domainBlacklist.empty() || !m_domainWhitelist.empty();
}

bool AdBlockFilter::isRedirect() const
{
    return m_redirect;
}

const QString &AdBlockFilter::getRedirectName() const
{
    return m_redirectName;
}

bool AdBlockFilter::isMatch(const QString &baseUrl, const QString &requestUrl, const QString &requestDomain, ElementType typeMask)
{
    if (m_disabled)
        return false;

    // Check for domain restrictions
    if (hasDomainRules() && !isDomainStyleMatch(baseUrl))
        return false;

    // Special cases
    if (typeMask == ElementType::InlineScript && !hasElementType(m_blockedTypes, ElementType::InlineScript))
        return false;
    if (hasElementType(m_blockedTypes, ElementType::ThirdParty) && !hasElementType(typeMask, ElementType::ThirdParty))
        return false;
    if (hasElementType(m_allowedTypes, ElementType::ThirdParty) && hasElementType(typeMask, ElementType::ThirdParty))
        return false;

    bool match = m_matchAll;

    if (!match)
    {
        Qt::CaseSensitivity caseSensitivity = m_matchCase ? Qt::CaseSensitive : Qt::CaseInsensitive;
        switch (m_category)
        {
            case FilterCategory::Stylesheet:    // Handled in AdBlockManager
            case FilterCategory::StylesheetJS:
            case FilterCategory::StylesheetCustom:
                return false;
            case FilterCategory::Domain:
                match = isDomainMatch(requestDomain, m_evalString);
                break;
            case FilterCategory::DomainStart:
                match = isDomainStartMatch(requestUrl, URL(requestDomain).getSecondLevelDomain());
                break;
            case FilterCategory::StringStartMatch:
                match = requestUrl.startsWith(m_evalString, caseSensitivity);
                break;
            case FilterCategory::StringEndMatch:
                match = requestUrl.endsWith(m_evalString, caseSensitivity);
                break;
            case FilterCategory::StringExactMatch:
                match = (requestUrl.compare(m_evalString, caseSensitivity) == 0);
                break;
            case FilterCategory::StringContains:
            {
                QString haystack = (m_matchCase ? requestUrl : requestUrl.toLower());
                std::wstring haystackWStr = haystack.toStdWString();
                match = FastHash::isMatch(m_needleWStr, haystackWStr, m_evalStringHash, m_differenceHash);
                break;
            }
            case FilterCategory::RegExp:
                match = m_regExp->match(requestUrl).hasMatch();
                break;
            default:
                break;
        }
    }

    if (match)
    {
        // Check for element type restrictions (in specific order)
        std::array<ElementType, 13> elemTypes = {  ElementType::XMLHTTPRequest,  ElementType::Document,   ElementType::Object,
                                                   ElementType::Subdocument,     ElementType::Image,      ElementType::Script,
                                                   ElementType::Stylesheet,      ElementType::WebSocket,  ElementType::ObjectSubrequest,
                                                   ElementType::InlineScript,    ElementType::Ping,       ElementType::CSP,
                                                   ElementType::Other };

        for (std::size_t i = 0; i < elemTypes.size(); ++i)
        {
            ElementType currentType = elemTypes[i];
            bool isRequestOfType = hasElementType(typeMask, currentType);
            if (hasElementType(m_allowedTypes, currentType) && isRequestOfType)
                return false;
            if (hasElementType(m_blockedTypes, currentType) && isRequestOfType)
                return true;
        }


        //ElementType::ThirdParty | ElementType::MatchCase | ElementType::Collapse
        ElementType ignoreTypeMask = static_cast<ElementType>(~0x00038000ULL);
        if ((m_blockedTypes & ignoreTypeMask) != ElementType::None)
            return false;
        //if (m_blockedTypes != ElementType::None && !hasElementType(m_blockedTypes, ElementType::ThirdParty))
        //    return false;
    }

    return match;
}

bool AdBlockFilter::isDomainStyleMatch(const QString &domain)
{
    if (m_disabled || domain.isEmpty())
        return false;

    if (m_domainBlacklist.empty() && m_domainWhitelist.empty())
        return true;

    for (const QString &d : m_domainWhitelist)
    {
        if (isDomainMatch(domain, d))
            return false;
    }
    for (const QString &d : m_domainBlacklist)
    {
        if (isDomainMatch(domain, d))
            return true;
    }

    return false;
}

void AdBlockFilter::addDomainToWhitelist(const QString &domainStr)
{
    m_domainWhitelist.insert(domainStr);
}

void AdBlockFilter::addDomainToBlacklist(const QString &domainStr)
{
    m_domainBlacklist.insert(domainStr);
}

void AdBlockFilter::setEvalString(const QString &evalString)
{
    m_evalString = evalString;
}

bool AdBlockFilter::isDomainMatch(QString base, const QString &domainStr) const
{
    // Check if domain match is being performed on an entity filter
    if (domainStr.endsWith(QChar('.')))
        base = base.left(base.lastIndexOf(QChar('.')) + 1);

    if (base.compare(domainStr) == 0)
        return true;

    if (!base.endsWith(domainStr))
        return false;

    int evalIdx = base.indexOf(domainStr); //domainStr.indexOf(base);
    return (evalIdx > 0 && base.at(evalIdx - 1) == QChar('.'));
}

bool AdBlockFilter::isDomainStartMatch(const QString &requestUrl, const QString &secondLevelDomain) const
{
    Qt::CaseSensitivity caseSensitivity = m_matchCase ? Qt::CaseSensitive : Qt::CaseInsensitive;
    int matchIdx = requestUrl.indexOf(m_evalString, 0, caseSensitivity);
    if (matchIdx > 0)
    {
        QChar c = requestUrl[matchIdx - 1];
        const bool validChar = c == QChar('.') || c == QChar('/');
        return validChar || m_evalString.contains(secondLevelDomain, caseSensitivity);
    }
    return false;
}

void AdBlockFilter::hashEvalString()
{
    m_needleWStr = m_evalString.toStdWString();
    m_differenceHash = FastHash::getDifferenceHash(static_cast<quint64>(m_evalString.size()));
    m_evalStringHash = FastHash::getNeedleHash(m_needleWStr);
}

void AdBlockFilter::setContentSecurityPolicy(const QString &csp)
{
    m_contentSecurityPolicy = csp;
}
