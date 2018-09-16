#include <algorithm>
#include <array>
#include "AdBlockFilter.h"
#include "Bitfield.h"
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
    m_evalStringHash(0)
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
    m_evalStringHash(other.m_evalStringHash)
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
    m_evalStringHash(other.m_evalStringHash)
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
                match = filterContains(haystack);
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
        // Check for domain restrictions
        if (hasDomainRules() && !isDomainStyleMatch(baseUrl)) //!isDomainMatch(m_evalString, baseUrl))
            return false;

        // Check for element type restrictions (in specific order)
        std::array<ElementType, 13> elemTypes = {{ ElementType::XMLHTTPRequest,  ElementType::Document,   ElementType::Object,
                                                   ElementType::Subdocument,     ElementType::Image,      ElementType::Script,
                                                   ElementType::Stylesheet,      ElementType::WebSocket,  ElementType::ObjectSubrequest,
                                                   ElementType::InlineScript,    ElementType::Ping,       ElementType::CSP,
                                                   ElementType::Other }};

        for (std::size_t i = 0; i < elemTypes.size(); ++i)
        {
            ElementType currentType = elemTypes[i];
            bool isRequestOfType = hasElementType(typeMask, currentType);
            if (hasElementType(m_allowedTypes, currentType) && isRequestOfType)
                return false;
            if (hasElementType(m_blockedTypes, currentType) && isRequestOfType)
                return true;
        }

        if (m_blockedTypes != ElementType::None)
            return false;
    }

    return match;
}

bool AdBlockFilter::isDomainStyleMatch(const QString &domain)
{
    if (m_disabled)
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
        if (!secondLevelDomain.isEmpty())
            return m_evalString.contains(secondLevelDomain, caseSensitivity) && validChar;
        else
            return validChar;
    }
    return false;
}

bool AdBlockFilter::filterContains(const QString &haystack) const
{
    static const quint64 radixLength = 256ULL;
    static const quint64 prime = 72057594037927931ULL;

    const int needleLength = m_evalString.size();
    const int haystackLength = haystack.size();

    if (needleLength > haystackLength)
        return false;
    if (needleLength == 0)
        return true;

    const QChar *needlePtr = m_evalString.constData();
    const QChar *haystackPtr = haystack.constData();

    int i, j;
    quint64 t = 0;
    quint64 h = 1;

    // The value of h would be "pow(d, M-1)%q"
    for (i = 0; i < needleLength - 1; ++i)
        h = (h * radixLength) % prime;

    // Calculate the hash value of first window of text
    for (i = 0; i < needleLength; ++i)
        t = (radixLength * t + ((haystackPtr + i)->toLatin1())) % prime;

    for (i = 0; i <= haystackLength - needleLength; ++i)
    {
        if (m_evalStringHash == t)
        {
            for (j = 0; j < needleLength; j++)
            {
                if ((haystackPtr + i + j)->toLatin1() != (needlePtr + j)->toLatin1())
                    break;
            }

            if (j == needleLength)
                return true;
        }

        if (i < haystackLength - needleLength)
        {
            t = (radixLength * (t - (h * (haystackPtr + i)->toLatin1()))
                 + (haystackPtr + i + needleLength)->toLatin1()) % prime;
        }
    }

    return false;
}

void AdBlockFilter::hashEvalString()
{
    const int needleLength = m_evalString.size();
    const QChar *needlePtr = m_evalString.constData();

    const quint64 radixLength = 256ULL;
    const quint64 prime = 72057594037927931ULL;

    m_differenceHash = quPow(radixLength, static_cast<quint64>(needleLength - 1)) % prime;

    for (int index = 0; index < needleLength; ++index)
        m_evalStringHash = (radixLength * m_evalStringHash + (needlePtr + index)->toLatin1()) % prime;
}

void AdBlockFilter::setContentSecurityPolicy(const QString &csp)
{
    m_contentSecurityPolicy = csp;
}
