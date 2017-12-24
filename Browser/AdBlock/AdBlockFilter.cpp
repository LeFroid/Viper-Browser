#include "AdBlockFilter.h"
#include "Bitfield.h"

QHash<QString, ElementType> eOptionMap = {
    { "script", ElementType::Script },           { "image", ElementType::Image },                   { "stylesheet", ElementType::Stylesheet },
    { "object", ElementType::Object },           { "xmlhttprequest", ElementType::XMLHTTPRequest }, { "object-subrequest", ElementType::ObjectSubrequest },
    { "subdocument", ElementType::Subdocument }, { "ping", ElementType::Ping },                     { "websocket", ElementType::WebSocket },
    { "webrtc", ElementType::WebRTC },           { "document", ElementType::Document },             { "elemhide", ElementType::ElemHide },
    { "generichide", ElementType::GenericHide }, { "genericblock", ElementType::GenericBlock },     { "popup", ElementType::PopUp },
    { "third-party", ElementType::ThirdParty},   { "match-case", ElementType::MatchCase },          { "collapse", ElementType::Collapse },
    { "other", ElementType::Other }
};

AdBlockFilter::AdBlockFilter() :
    m_category(FilterCategory::None),
    m_ruleString(),
    m_evalString(),
    m_exception(false),
    m_important(false),
    m_allowedTypes(ElementType::None),
    m_blockedTypes(ElementType::None),
    m_matchCase(false),
    m_matchAll(false),
    m_domainBlacklist(),
    m_domainWhitelist(),
    m_regExp(nullptr)
{
}

AdBlockFilter::AdBlockFilter(const QString &rule) :
    m_category(FilterCategory::None),
    m_ruleString(rule),
    m_evalString(),
    m_exception(false),
    m_important(false),
    m_allowedTypes(ElementType::None),
    m_blockedTypes(ElementType::None),
    m_matchCase(false),
    m_matchAll(false),
    m_domainBlacklist(),
    m_domainWhitelist(),
    m_regExp(nullptr)
{
    // Only bother parsing the rule if the given string is not empty and is not a comment
    if (!rule.isEmpty() && !rule.startsWith('!'))
        parseRule();
}

AdBlockFilter::AdBlockFilter(const AdBlockFilter &other) :
    m_category(other.m_category),
    m_ruleString(other.m_ruleString),
    m_evalString(other.m_evalString),
    m_exception(other.m_exception),
    m_important(other.m_important),
    m_allowedTypes(other.m_allowedTypes),
    m_blockedTypes(other.m_blockedTypes),
    m_matchCase(other.m_matchCase),
    m_matchAll(other.m_matchAll),
    m_domainBlacklist(other.m_domainBlacklist),
    m_domainWhitelist(other.m_domainWhitelist),
    m_regExp(nullptr)
{
    m_regExp = (other.m_regExp ? std::make_unique<QRegularExpression>(*other.m_regExp) : nullptr);
}

AdBlockFilter::AdBlockFilter(AdBlockFilter &&other) :
    m_category(other.m_category),
    m_ruleString(other.m_ruleString),
    m_evalString(other.m_evalString),
    m_exception(other.m_exception),
    m_important(other.m_important),
    m_allowedTypes(other.m_allowedTypes),
    m_blockedTypes(other.m_blockedTypes),
    m_matchCase(other.m_matchCase),
    m_matchAll(other.m_matchAll),
    m_domainBlacklist(std::move(other.m_domainBlacklist)),
    m_domainWhitelist(std::move(other.m_domainWhitelist)),
    m_regExp(std::move(other.m_regExp))
{
}

AdBlockFilter &AdBlockFilter::operator =(const AdBlockFilter &other)
{
    m_category = other.m_category;
    m_ruleString = other.m_ruleString;
    m_evalString = other.m_evalString;
    m_exception = other.m_exception;
    m_important = other.m_important;
    m_allowedTypes = other.m_allowedTypes;
    m_blockedTypes = other.m_blockedTypes;
    m_matchCase = other.m_matchCase;
    m_matchAll = other.m_matchAll;
    m_domainBlacklist = other.m_domainBlacklist;
    m_domainWhitelist = other.m_domainWhitelist;
    m_regExp = (other.m_regExp ? std::make_unique<QRegularExpression>(*other.m_regExp) : nullptr);

    return *this;
}

AdBlockFilter &AdBlockFilter::operator =(AdBlockFilter &&other)
{
    if(this != &other)
    {
        m_category = other.m_category;
        m_ruleString = other.m_ruleString;
        m_evalString = other.m_evalString;
        m_exception = other.m_exception;
        m_important = other.m_important;
        m_allowedTypes = other.m_allowedTypes;
        m_blockedTypes = other.m_blockedTypes;
        m_matchCase = other.m_matchCase;
        m_matchAll = other.m_matchAll;
        m_domainBlacklist = std::move(other.m_domainBlacklist);
        m_domainWhitelist = std::move(other.m_domainWhitelist);
        m_regExp = std::move(other.m_regExp);
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
    // Only bother parsing the rule if the given string is not empty and is not a comment
    if (!rule.isEmpty() && !rule.startsWith('!'))
    {
        m_allowedTypes = ElementType::None;
        m_blockedTypes = ElementType::None;
        m_matchCase = false;
        m_matchAll = false;
        m_domainBlacklist.clear();
        m_domainWhitelist.clear();
        m_regExp.reset();
        m_ruleString = rule;
        m_evalString = QString();
        parseRule();
    }
}

const QString &AdBlockFilter::getRule() const
{
    return m_ruleString;
}

const QString &AdBlockFilter::getEvalString() const
{
    return m_evalString;
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

bool AdBlockFilter::isMatch(const QString &baseUrl, const QString &requestUrl, const QString &requestDomain, ElementType typeMask)
{
    bool match = m_matchAll;

    if (!match)
    {
        Qt::CaseSensitivity caseSensitivity = m_matchCase ? Qt::CaseSensitive : Qt::CaseInsensitive;
        switch (m_category)
        {
            case FilterCategory::Stylesheet: //handled more directly in ad block manager
            case FilterCategory::StylesheetJS:
                return false;
            case FilterCategory::Domain:
                match = isDomainMatch(m_evalString, requestDomain);
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
        if (hasDomainRules() && !isDomainMatch(m_evalString, baseUrl))
            return false;

        // Check for XMLHttpRequest restrictions
        bool isElemType = (typeMask & ElementType::XMLHTTPRequest) == ElementType::XMLHTTPRequest;
        if (((m_allowedTypes & ElementType::XMLHTTPRequest) == ElementType::XMLHTTPRequest) && isElemType)
            return false;
        if (((m_blockedTypes & ElementType::XMLHTTPRequest) == ElementType::XMLHTTPRequest) && isElemType)
            return true;

        // Check for document restrictions
        isElemType = (typeMask & ElementType::Document) == ElementType::Document;
        if (((m_allowedTypes & ElementType::Document) == ElementType::Document) && isElemType)
            return false;
        if (((m_blockedTypes & ElementType::Document) == ElementType::Document) && isElemType)
            return true;

        // Check for object restrictions
        isElemType = (typeMask & ElementType::Object) == ElementType::Object;
        if (((m_allowedTypes & ElementType::Object) == ElementType::Object) && isElemType)
            return false;
        if (((m_blockedTypes & ElementType::Object) == ElementType::Object) && isElemType)
            return true;

        // Check for subdocument restrictions
        isElemType = (typeMask & ElementType::Subdocument) == ElementType::Subdocument;
        if (((m_allowedTypes & ElementType::Subdocument) == ElementType::Subdocument) && isElemType)
            return false;
        if (((m_blockedTypes & ElementType::Subdocument) == ElementType::Subdocument) && isElemType)
            return true;

        // Check for third-party restrictions. If third-party in type mask, request is known to be third party
        isElemType = (typeMask & ElementType::ThirdParty) == ElementType::ThirdParty;
        if (((m_allowedTypes & ElementType::ThirdParty) == ElementType::ThirdParty) && isElemType)
            return false;
        if (((m_blockedTypes & ElementType::ThirdParty) == ElementType::ThirdParty) && isElemType)
            return true;

        // Check for image restrictions
        isElemType = (typeMask & ElementType::Image) == ElementType::Image;
        if (((m_allowedTypes & ElementType::Image) == ElementType::Image) && isElemType)
            return false;
        if (((m_blockedTypes & ElementType::Image) == ElementType::Image) && isElemType)
            return true;

        // Check for script restrictions
        isElemType = (typeMask & ElementType::Script) == ElementType::Script;
        if (((m_allowedTypes & ElementType::Script) == ElementType::Script) && isElemType)
            return false;
        if (((m_blockedTypes & ElementType::Script) == ElementType::Script) && isElemType)
            return true;
    }

    return match;
}

bool AdBlockFilter::isDomainStyleMatch(const QString &domain)
{
    // this method is failing
    if (m_domainBlacklist.isEmpty())
    {
        for (const QString &d : m_domainWhitelist)
        {
            if (isDomainMatch(domain, d))
                return false;
        }
    }
    else if (m_domainWhitelist.isEmpty())
    {
        for (const QString &d : m_domainBlacklist)
        {
            if (isDomainMatch(domain, d))
                return true;
        }
        return false;
    }
    else
    {
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
    }
    return false;
}

void AdBlockFilter::addDomainToWhitelist(const QString &domainStr)
{
    m_domainWhitelist.insert(domainStr);
}

bool AdBlockFilter::isDomainMatch(QString base, const QString &domainStr) const
{
    // Check if domain match is being performed on an entity filter
    if (domainStr.endsWith(QChar('.')))
        base = base.left(base.lastIndexOf(QChar('.') + 1));

    if (base.compare(domainStr) == 0)
        return true;

    if (!domainStr.endsWith(base))
        return false;

    int evalIdx = domainStr.indexOf(base);
    return (evalIdx > 0 && domainStr.at(evalIdx - 1) == QChar('.'));
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

void AdBlockFilter::parseRule()
{
    QString rule = m_ruleString;
    int pos = 0;

    // Check if CSS rule
    if ((pos = rule.indexOf("##")) >= 0)
    {
        m_category = FilterCategory::Stylesheet;

        // Fill domain blacklist
        if (pos > 0)
            parseDomains(rule.left(pos), QChar(','));

        m_evalString = m_ruleString.mid(pos + 2);
        parseCosmeticOptions();
        return;
    }

    // Check if CSS exception
    if ((pos = rule.indexOf("#@#")) >= 0)
    {
        m_category = FilterCategory::Stylesheet;
        m_exception = true;

        // Fill domain whitelist
        if (pos > 0)
            parseDomains(rule.left(pos), QChar(','));

        m_evalString = m_ruleString.mid(pos + 3);
        parseCosmeticOptions();
        return;
    }

    // Check if the rule is an exception
    if (rule.startsWith(QString("@@")))
    {
        m_exception = true;
        rule = rule.mid(2);
    }

    // Check if filter options are set
    if ((pos = rule.indexOf(QChar('$'))) >= 0)
    {
        parseOptions(rule.mid(pos + 1));
        rule = rule.left(pos);
    }

    // Check if rule is a regular expression
    if (rule.startsWith('/') && rule.endsWith('/'))
    {
        m_category = FilterCategory::RegExp;

        rule = rule.mid(1);
        rule = rule.left(rule.size() - 1);

        QRegularExpression::PatternOptions options =
                (m_matchCase ? QRegularExpression::NoPatternOption : QRegularExpression::CaseInsensitiveOption);
        m_regExp = std::make_unique<QRegularExpression>(rule.replace("\\", "\\\\"), options);
        return;
    }

    // Remove any leading wildcard
    if (rule.startsWith('*'))
        rule = rule.mid(1);

    // Check if filter is an entity filter before removing trailing wildcard
    //const bool isEntityFilter = rule.endsWith(QStringLiteral(".*")) || rule.endsWith(QStringLiteral(".*^"));

    // Remove trailing wildcard
    if (rule.endsWith('*'))
        rule = rule.left(rule.size() - 1);

    // Check for domain matching rule
    if (rule.startsWith("||") && rule.endsWith('^'))
    {
        bool isDomainCategory = true;

        for (int i = 2; i < rule.size() - 1; ++i)
        {
            if (!isDomainCategory)
                break;

            switch (rule.at(i).toLatin1())
            {
                case '/':
                case ':':
                case '?':
                case '=':
                case '&':
                case '*':
                    isDomainCategory = false;
                    break;
                default:
                    break;
            }
        }

        if (isDomainCategory)
        {
            m_evalString = rule.mid(2);
            m_evalString = m_evalString.left(m_evalString.size() - 1);
            m_category = FilterCategory::Domain;
            return;
        }
    }

    // Check if a regular expression might be needed
    const bool maybeRegExp = rule.contains('*') || rule.contains('^');

    // Domain start match
    if (rule.startsWith(QStringLiteral("||")) && !maybeRegExp)
    {
        m_category = FilterCategory::DomainStart;
        m_evalString = rule.mid(2);

        if (!m_matchCase)
            m_evalString = m_evalString.toLower();
        return;
    }

    // String start match
    if (rule.startsWith('|') && rule.at(1) != '|' && !maybeRegExp)
    {
        m_category = FilterCategory::StringStartMatch;
        rule = rule.mid(1);
    }

    // String end match (or exact match)
    if (rule.endsWith('|') && !maybeRegExp)
    {
        if (m_category == FilterCategory::StringStartMatch)
            m_category = FilterCategory::StringExactMatch;
        else
            m_category = FilterCategory::StringEndMatch;

        rule = rule.left(rule.size() - 1);
    }

    //if (rule.contains('*') || rule.contains('^') || rule.contains('|'))
    if (maybeRegExp || rule.contains(QChar('|')))
    {
        QRegularExpression::PatternOptions options =
                (m_matchCase ? QRegularExpression::NoPatternOption : QRegularExpression::CaseInsensitiveOption);
        m_regExp = std::make_unique<QRegularExpression>(parseRegExp(rule), options);
        m_category = FilterCategory::RegExp;
        return;
    }

    // If no category set by now, it is a string contains type
    if (m_category == FilterCategory::None)
        m_category = FilterCategory::StringContains;

    m_evalString = rule;

    if (m_evalString.isEmpty())
        m_matchAll = true;

    if (!m_matchCase)
        m_evalString = m_evalString.toLower();
}

void AdBlockFilter::parseDomains(const QString &domainString, QChar delimiter)
{
    QStringList domainList = domainString.split(delimiter, QString::SkipEmptyParts);
    for (QString &domain : domainList)
    {
        // Check if domain is an entity filter type
        if (domain.endsWith(QStringLiteral(".*")))
            domain = domain.left(domain.size() - 1);

        if (domain.at(0) == QChar('~'))
            m_domainWhitelist.insert(domain.mid(1));
        else
            m_domainBlacklist.insert(domain);
    }
}

void AdBlockFilter::parseOptions(const QString &optionString)
{
    QStringList optionsList = optionString.split(QChar(','), QString::SkipEmptyParts);
    for (const QString &option : optionsList)
    {
        const bool optionException = (option.at(0) == QChar('~'));
        QString name = optionException ? option.mid(1) : option;

        auto it = eOptionMap.find(name);
        if (it != eOptionMap.end())
        {
            ElementType elemType = it.value();
            if (elemType == ElementType::MatchCase)
                m_matchCase = true;

            if (optionException)
                m_allowedTypes |= elemType;
            else
                m_blockedTypes |= elemType;
        }
        else if (option.startsWith("domain="))
        {
            parseDomains(option.mid(7), QChar('|'));
        }
        // Handle options specific to uBlock Origin
        else if (option.compare(QStringLiteral("first-party")) == 0)
            m_blockedTypes |= ElementType::ThirdParty;
        else if (!m_exception && (option.compare(QStringLiteral("important")) == 0))
            m_important = true;
    }
}

void AdBlockFilter::parseCosmeticOptions()
{
    //TODO: support all other types of cosmetic filter options
    //See https://github.com/gorhill/uBlock/wiki/Procedural-cosmetic-filters for reference

    // Check for :xpath(...)
    int xpathIdx = m_evalString.indexOf(QStringLiteral(":xpath("));
    if (xpathIdx >= 0 && (!m_domainBlacklist.empty() || !m_domainWhitelist.empty()))
    {
        m_category = FilterCategory::StylesheetJS;

        QString xpathArg = m_evalString.mid(xpathIdx + 7);
        xpathArg = xpathArg.left(xpathArg.size() - 1);

        m_evalString = m_evalString.left(xpathIdx);
        if (m_evalString.isEmpty())
            m_evalString = QStringLiteral("*");

        QString javascript = QString("var output = [], i, j; \n var xpathExpr = document.createExpression('%1', null); "
                                     "var xpathResult = null; var nodes = document.querySelectorAll('%2'); for ("
                                     "i = 0; i < nodes.length; ++i) { xpathResult = xpathExpr.evaluate(nodes[i], "
                                     "XPathResult.UNORDERED_NODE_SNAPSHOT_TYPE, xpathResult); j = xpathResult.snapshotLength; "
                                     "while (j--) { xpathResult.snapshotItem(j).style.display = 'none'; } } ").arg(xpathArg).arg(m_evalString);
        m_evalString = javascript;
    }
}

QString AdBlockFilter::parseRegExp(const QString &regExpString)
{
    int strSize = regExpString.size();

    QString replacement;
    replacement.reserve(strSize);

    QChar c;
    bool usedStar = false;
    for (int i = 0; i < strSize; ++i)
    {
        c = regExpString.at(i);

        switch (c.toLatin1())
        {
            case '*':
            {
                if (!usedStar)
                {
                    replacement.append(QStringLiteral("[^ ]*?"));
                    usedStar = true;
                }
                break;
            }
            case '^':
            {
                replacement.append(QStringLiteral("(?:[^%.a-zA-Z0-9_-]|$)"));
                break;
            }
            case '|':
            {
                if (i == 0)
                {
                    if (regExpString.at(1) == '|')
                    {
                        replacement.append(QStringLiteral("^[a-z-]+://(?:[^\\/?#]+\\.)?"));
                        ++i;
                    }
                    else
                        replacement.append(QChar('^'));
                }
                else if (i == strSize - 1)
                    replacement.append(QChar('$'));

                break;
            }
            default:
            {
                if (c.isLetterOrNumber() || c.isMark() || c == '_')
                    replacement.append(c);
                else
                    replacement.append(QLatin1Char('\\') + c);

                break;
            }
        }
    }
    return replacement;
}

quint64 AdBlockFilter::quPow(quint64 base, quint64 exp) const
{
    quint64 result = 1;
    while (exp)
    {
        if (exp & 1)
            result *= base;
        exp >>= 1;
        base *= base;
    }
    return result;
}

bool AdBlockFilter::filterContains(const QString &haystack) const
{
    if (m_evalString.size() > haystack.size())
        return false;

    if (m_evalString.isEmpty())
        return true;

    static const quint64 radixLength = 256ULL;
    static const quint64 prime = 72057594037927931ULL;

    int needleLength = m_evalString.size();
    int haystackLength = haystack.size();
    int lastIndex = haystackLength - needleLength;

    quint64 differenceHash = quPow(radixLength, static_cast<quint64>(needleLength - 1)) % prime;

    size_t needleHash = 0;
    size_t firstHaystackHash = 0;

    int index;

    // preprocessing
    for(index = 0; index < needleLength; index++)
    {
        needleHash = (radixLength * needleHash + m_evalString[index].toLatin1()) % prime;
        firstHaystackHash = (radixLength * firstHaystackHash + haystack[index].toLatin1()) % prime;
    }

    std::vector<quint64> haystackHashes;
    haystackHashes.reserve(lastIndex + 1);
    haystackHashes.push_back(firstHaystackHash);

    // matching
    for(index = 0; index <= lastIndex; index++)
    {
        if(needleHash == haystackHashes[index])
        {
           int j;
           for (j = 0; j < needleLength && m_evalString[j] == haystack[index + j]; ++j);
           if (j == needleLength)
               return true;
        }

        if(index < lastIndex)
        {
            quint64 newHaystackHash =
                    (radixLength * (haystackHashes[index] - haystack[index].toLatin1() * differenceHash)
                     + haystack[index + needleLength].toLatin1()) % prime;
            haystackHashes.push_back(newHaystackHash);
        }
    }

    return false;
}
