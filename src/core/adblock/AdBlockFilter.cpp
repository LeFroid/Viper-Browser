#include "AdBlockFilter.h"
#include "Bitfield.h"
#include "FastHash.h"
#include "URL.h"

#include <algorithm>
#include <array>

namespace adblock
{

Filter::Filter(const QString &rule) :
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

Filter::Filter(const Filter &other) :
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

Filter::Filter(Filter &&other) noexcept :
    m_category(other.m_category),
    m_ruleString(std::move(other.m_ruleString)),
    m_evalString(std::move(other.m_evalString)),
    m_contentSecurityPolicy(std::move(other.m_contentSecurityPolicy)),
    m_exception(other.m_exception),
    m_important(other.m_important),
    m_disabled(other.m_disabled),
    m_redirect(other.m_redirect),
    m_redirectName(std::move(other.m_redirectName)),
    m_allowedTypes(other.m_allowedTypes),
    m_blockedTypes(other.m_blockedTypes),
    m_matchCase(other.m_matchCase),
    m_matchAll(other.m_matchAll),
    m_domainBlacklist(std::move(other.m_domainBlacklist)),
    m_domainWhitelist(std::move(other.m_domainWhitelist)),
    m_regExp(std::move(other.m_regExp)),
    m_differenceHash(other.m_differenceHash),
    m_evalStringHash(other.m_evalStringHash),
    m_needleWStr(std::move(other.m_needleWStr))
{
}

Filter &Filter::operator =(const Filter &other)
{
    if (this != &other)
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

Filter &Filter::operator =(Filter &&other) noexcept
{
    if (this != &other)
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

Filter::~Filter()
{
}

FilterCategory Filter::getCategory() const
{
    return m_category;
}

void Filter::setRule(const QString &rule)
{
    m_ruleString = rule;
}

const QString &Filter::getRule() const
{
    return m_ruleString;
}

const QString &Filter::getEvalString() const
{
    return m_evalString;
}

const QString &Filter::getContentSecurityPolicy() const
{
    return m_contentSecurityPolicy;
}

bool Filter::isException() const
{
    return m_exception;
}

bool Filter::isImportant() const
{
    return m_important;
}

bool Filter::hasDomainRules() const
{
    return !m_domainBlacklist.empty() || !m_domainWhitelist.empty();
}

bool Filter::isRedirect() const
{
    return m_redirect;
}

const QString &Filter::getRedirectName() const
{
    return m_redirectName;
}

bool Filter::isMatch(const QString &baseUrl, const QString &requestUrl, const QString &requestDomain, ElementType typeMask) const
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
            case FilterCategory::Scriptlet:
                return false;
            case FilterCategory::Domain:
                match = isDomainMatch(requestDomain, m_evalString);
                break;
            case FilterCategory::DomainStart:
                match = isDomainStartMatch(requestUrl, requestDomain);
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
        static constexpr std::array<ElementType, 13> elemTypes = {  ElementType::XMLHTTPRequest,  ElementType::Document,   ElementType::Object,
                                                   ElementType::Subdocument,     ElementType::Image,      ElementType::Script,
                                                   ElementType::Stylesheet,      ElementType::WebSocket,  ElementType::ObjectSubrequest,
                                                   ElementType::InlineScript,    ElementType::Ping,       ElementType::CSP,
                                                   ElementType::Other };

        // bool allowForHost = () => { if (m_denyAllowHosts.empty()) { true } else { return domainOf(requestUrl) in m_denyAllowHosts } };
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
    }

    return match;
}

bool Filter::isDomainStyleMatch(const QString &domain) const
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

    // "Block all" except for whitelisted domains. Consider this a match
    if (!m_domainWhitelist.empty() && m_domainBlacklist.empty())
        return true;

    for (const QString &d : m_domainBlacklist)
    {
        if (isDomainMatch(domain, d))
            return true;
    }

    return false;
}

void Filter::addDomainToWhitelist(const QString &domainStr)
{
    m_domainWhitelist.insert(domainStr);
}

void Filter::addDomainToBlacklist(const QString &domainStr)
{
    m_domainBlacklist.insert(domainStr);
}

void Filter::setEvalString(const QString &evalString)
{
    m_evalString = evalString;
}

bool Filter::isDomainMatch(QString base, const QString &domainStr) const
{
    // Check if domain match is being performed on an entity filter
    if (domainStr.endsWith(QChar('.')))
        base = base.left(base.lastIndexOf(QChar('.')) + 1);

    if (base.compare(domainStr) == 0)
        return true;

    if (!base.endsWith(domainStr))
        return false;

    int evalIdx = base.indexOf(domainStr);
    return evalIdx == 0 || (evalIdx > 0 && base.at(evalIdx - 1) == QChar('.'));
}

bool Filter::isDomainStartMatch(const QString &requestUrl, const QString &requestDomain) const
{
    Qt::CaseSensitivity caseSensitivity = m_matchCase ? Qt::CaseSensitive : Qt::CaseInsensitive;
    int matchIdx = requestUrl.indexOf(m_evalString, 0, caseSensitivity);
    if (matchIdx > 0)
    {
        QChar c = requestUrl[matchIdx - 1];
        const bool validChar = c == QChar('.') || c == QChar('/');
        return validChar || m_evalString.contains(requestDomain, caseSensitivity);
    }
    return false;
}

void Filter::hashEvalString()
{
    if (m_matchAll || m_evalString.isEmpty())
        return;

    m_needleWStr = m_evalString.toStdWString();
    m_differenceHash = FastHash::getDifferenceHash(static_cast<quint64>(m_evalString.size()));
    m_evalStringHash = FastHash::getNeedleHash(m_needleWStr);
}

void Filter::setContentSecurityPolicy(const QString &csp)
{
    m_contentSecurityPolicy = csp;
}

}
