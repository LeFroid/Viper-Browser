#include <algorithm>
#include "AdBlockFilter.h"
#include "AdBlockManager.h"
#include "Bitfield.h"

QHash<QString, ElementType> eOptionMap = {
    { QStringLiteral("script"), ElementType::Script },                 { QStringLiteral("image"), ElementType::Image },
    { QStringLiteral("stylesheet"), ElementType::Stylesheet },         { QStringLiteral("object"), ElementType::Object },
    { QStringLiteral("xmlhttprequest"), ElementType::XMLHTTPRequest }, { QStringLiteral("object-subrequest"), ElementType::ObjectSubrequest },
    { QStringLiteral("subdocument"), ElementType::Subdocument },       { QStringLiteral("ping"), ElementType::Ping },
    { QStringLiteral("websocket"), ElementType::WebSocket },           { QStringLiteral("webrtc"), ElementType::WebRTC },
    { QStringLiteral("document"), ElementType::Document },             { QStringLiteral("elemhide"), ElementType::ElemHide },
    { QStringLiteral("generichide"), ElementType::GenericHide },       { QStringLiteral("genericblock"), ElementType::GenericBlock },
    { QStringLiteral("popup"), ElementType::PopUp },                   { QStringLiteral("third-party"), ElementType::ThirdParty},
    { QStringLiteral("match-case"), ElementType::MatchCase },          { QStringLiteral("collapse"), ElementType::Collapse },
    { QStringLiteral("badfilter"), ElementType::BadFilter },           { QStringLiteral("other"), ElementType::Other }
};

AdBlockFilter::AdBlockFilter() :
    m_category(FilterCategory::None),
    m_ruleString(),
    m_evalString(),
    m_exception(false),
    m_important(false),
    m_disabled(false),
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
    m_disabled(false),
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
    m_disabled(other.m_disabled),
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
    m_disabled(other.m_disabled),
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
    m_disabled = other.m_disabled;
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
        m_disabled = other.m_disabled;
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
    if (m_disabled)
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

        // Check for element type restrictions (in specific order)
        std::array<ElementType, 8> elemTypes = {{ ElementType::XMLHTTPRequest, ElementType::Document,   ElementType::Object,
                                                 ElementType::Subdocument,    ElementType::ThirdParty, ElementType::Image,
                                                 ElementType::Script,         ElementType::WebSocket }};

        for (std::size_t i = 0; i < elemTypes.size(); ++i)
        {
            ElementType currentType = elemTypes[i];
            bool isRequestOfType = hasElementType(typeMask, currentType);
            if (hasElementType(m_allowedTypes, currentType) && isRequestOfType)
                return false;
            if (hasElementType(m_blockedTypes, currentType) && isRequestOfType)
                return true;
        }
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
    if (isStylesheetRule())
        return;

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

bool AdBlockFilter::isStylesheetRule()
{
    int pos = 0;
    QString rule = m_ruleString;

    // Check if CSS rule
    if ((pos = rule.indexOf("##")) >= 0)
    {
        m_category = FilterCategory::Stylesheet;

        // Fill domain blacklist
        if (pos > 0)
            parseDomains(rule.left(pos), QChar(','));

        m_evalString = m_ruleString.mid(pos + 2);

        // Check for custom stylesheets, cosmetic options, and/or script injection filter rules
        if (parseCustomStylesheet() || parseCosmeticOptions() || parseScriptInjection())
            return true;

        return true;
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

        // Check if filter is procedural cosmetic filter type
        //parseCosmeticOptions();

        return true;
    }

    return false;
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
            {
                // Don't allow exception filters to whitelist entire pages / disable ad block (uBlock origin setting)
                if (m_exception && elemType == ElementType::Document)
                    m_disabled = true;

                m_blockedTypes |= elemType;
            }
        }
        else if (option.startsWith(QStringLiteral("domain=")))
        {
            parseDomains(option.mid(7), QChar('|'));
        }
        // Handle options specific to uBlock Origin
        else if (option.compare(QStringLiteral("first-party")) == 0)
            m_blockedTypes |= ElementType::ThirdParty;
        else if (!m_exception && (option.compare(QStringLiteral("important")) == 0))
            m_important = true;
    }

    // Check if BadFilter option was set, and remove the badfilter string from original rule
    // (so the hash can be made and compared to other filters for removal)
    if (hasElementType(m_blockedTypes, ElementType::BadFilter))
    {
        if (m_ruleString.endsWith(QStringLiteral(",badfilter")))
            m_ruleString = m_ruleString.left(m_ruleString.indexOf(QStringLiteral(",badfilter")));
        else if (m_ruleString.endsWith(QStringLiteral("$badfilter")))
            m_ruleString = m_ruleString.left(m_ruleString.indexOf(QStringLiteral("$badfilter")));
    }
}

bool AdBlockFilter::parseCosmeticOptions()
{
    if (!hasDomainRules())
        return false;

    // Replace -abp- terms with uBlock versions
    m_evalString.replace(QStringLiteral(":-abp-contains"), QStringLiteral(":has-text"));
    m_evalString.replace(QStringLiteral(":-abp-has"), QStringLiteral(":if"));

    // Check for :has(..)
    int hasIdx = m_evalString.indexOf(QStringLiteral(":has("));
    if (hasIdx >= 0)
    {
        QString hasArg = m_evalString.mid(hasIdx + 5);
        hasArg = hasArg.left(hasArg.size() - 1);

        m_evalString = m_evalString.left(hasIdx);
        if (m_evalString.isEmpty())
            return false;

        m_evalString = QString("hideIfHas('%1', '%2'); ").arg(m_evalString).arg(hasArg);
        m_category = FilterCategory::StylesheetJS;

        // :has cannot be chained, so exit the method at this point
        return true;
    }

    // Search for each chainable type and handle the first one to appear, as any other
    // chainable filter options will appear as an argument of the first
    std::vector<std::tuple<int, CosmeticFilter, int>> filters = getChainableFilters(m_evalString);
    if (filters.empty())
        return false;

    // Keep a copy of original evaluation string until processing is complete
    QString evalStr = m_evalString;

    // Process first item in filters container
    const std::tuple<int, CosmeticFilter, int> &p = filters.at(0);

    // Extract filter argument
    QString evalArg = evalStr.mid(std::get<0>(p) + std::get<2>(p));
    evalArg = evalArg.left(evalArg.lastIndexOf(QChar(')')));

    evalStr = evalStr.left(std::get<0>(p));
    if (std::get<1>(p) == CosmeticFilter::XPath && evalStr.isEmpty())
        evalStr = QStringLiteral("document");
    else if (evalStr.isEmpty())
        return false;

    switch (std::get<1>(p))
    {
        case CosmeticFilter::HasText:
        {
            // Place argument in quotes if not a regular expression
            if (!evalArg.startsWith(QChar('/'))
                    && (!evalArg.endsWith(QChar('/')) || !(evalArg.at(evalArg.size() - 2) == QChar('/'))))
            {
                evalArg = QString("'%1'").arg(evalArg);
            }

            m_evalString = QString("hideNodes(hasText('%1', %2)); ").arg(evalStr).arg(evalArg);
            break;
        }
        case CosmeticFilter::If:
        case CosmeticFilter::IfNot:
        {
            const bool isNegation = (std::get<1>(p) == CosmeticFilter::IfNot);
            // Check if anything within the :if(...) is a cosmetic filter, thus requiring the use of callbacks
            if (filters.size() > 1)
            {
                CosmeticJSCallback c = getTranslation(evalArg, filters);
                if (c.IsValid)
                {
                    if (isNegation)
                        m_evalString = QString("hideIfNotChain('%1', '%2', '%3', %4); ").arg(evalStr).arg(c.CallbackSubject).arg(c.CallbackTarget).arg(c.CallbackName);
                    else
                        m_evalString = QString("hideIfChain('%1', '%2', '%3', %4); ").arg(evalStr).arg(c.CallbackSubject).arg(c.CallbackTarget).arg(c.CallbackName);
                    m_category = FilterCategory::StylesheetJS;
                    return true;
                }
            }
            if (isNegation)
                m_evalString = QString("hideIfNotHas('%1', '%2'); ").arg(evalStr).arg(evalArg);
            else
                m_evalString = QString("hideIfHas('%1', '%2'); ").arg(evalStr).arg(evalArg);
            break;
        }
        case CosmeticFilter::MatchesCSS:
        {
            m_evalString = QString("hideNodes(matchesCSS('%1', '%2')); ").arg(evalStr).arg(evalArg);
            break;
        }
        case CosmeticFilter::MatchesCSSBefore:
        {
            evalStr.append(QStringLiteral(":before"));
            m_evalString = QString("hideNodes(matchesCSS('%1', '%2')); ").arg(evalStr).arg(evalArg);
            break;
        }
        case CosmeticFilter::MatchesCSSAfter:
        {
            evalStr.append(QStringLiteral(":after"));
            m_evalString = QString("hideNodes(matchesCSS('%1', '%2')); ").arg(evalStr).arg(evalArg);
            break;
        }
        case CosmeticFilter::XPath:
        {
            m_evalString = QString("hideNodes(doXPath('%1', '%2')); ").arg(evalStr).arg(evalArg);
            break;
        }
        default: return false;
    }
    m_category = FilterCategory::StylesheetJS;
    return true;
}

bool AdBlockFilter::parseCustomStylesheet()
{
    int styleIdx = m_evalString.indexOf(QStringLiteral(":style("));
    if (styleIdx < 0)
        return false;

    // Extract style, and convert evaluation string into format "selector { style }"
    QString style = m_evalString.mid(styleIdx + 7);
    style = style.left(style.lastIndexOf(QChar(')')));

    m_evalString = m_evalString.left(styleIdx).append(QString(" { %1 } ").arg(style));

    m_category = FilterCategory::StylesheetCustom;
    return true;
}

bool AdBlockFilter::parseScriptInjection()
{
    // Check if filter is used for script injection, and has at least 1 domain option
    if (!m_evalString.startsWith(QStringLiteral("script:inject(")) || !hasDomainRules())
        return false;

    m_category = FilterCategory::StylesheetJS;

    // Extract inner arguments and separate by ',' delimiter
    QString injectionStr = m_evalString.mid(14);
    injectionStr = injectionStr.left(injectionStr.lastIndexOf(QChar(')')));

    QStringList injectionArgs = injectionStr.split(QChar(','), QString::SkipEmptyParts);
    const QString &resourceName = injectionArgs.at(0);

    // Fetch resource from AdBlockManager and set value as m_evalString
    m_evalString = AdBlockManager::instance().getResource(resourceName);
    if (injectionArgs.size() < 2)
        return true;

    // For each item with index > 0 in injectionArgs list, replace first {{index}}
    // string in resource with injectionArgs[index]
    for (int i = 1; i < injectionArgs.size(); ++i)
    {
        QString arg = injectionArgs.at(i).trimmed();
        QString term = QString("{{%1}}").arg(i);
        int argIdx = m_evalString.indexOf(term);
        if (argIdx >= 0)
            m_evalString.replace(argIdx, term.size(), arg);
    }

    return true;
}

AdBlockFilter::CosmeticJSCallback AdBlockFilter::getTranslation(const QString &evalArg, const std::vector<std::tuple<int, CosmeticFilter, int>> &filters)
{
    CosmeticJSCallback result;

    const std::tuple<int, CosmeticFilter, int> &p = filters.at(0);

    // Attempt to extract information neccessary for callback invocation
    int argStrLen = evalArg.size();
    for (std::size_t i = 1; i < filters.size(); ++i)
    {
        auto const &p2 = filters.at(i);
        if (std::get<0>(p2) < std::get<0>(p) + std::get<2>(p) + argStrLen)
        {
            result.IsValid = true;

            // Extract selector and argument for nested filter
            int colonPos = std::get<0>(p2) - std::get<0>(p) - std::get<2>(p);
            int cosmeticLen = std::get<2>(p2);

            CosmeticFilter currFilter = std::get<1>(p2);
            switch (currFilter)
            {
                case CosmeticFilter::HasText:
                    result.CallbackName = QStringLiteral("hasText");
                    break;
                case CosmeticFilter::XPath:
                    result.CallbackName = QStringLiteral("doXPath");
                    break;
                case CosmeticFilter::MatchesCSS:
                case CosmeticFilter::MatchesCSSBefore:
                case CosmeticFilter::MatchesCSSAfter:
                    result.CallbackName = QStringLiteral("matchesCSS"); break;
                default: break; // If, IfNot and Has should not be nested
            }

            result.CallbackSubject = QString("%1").arg(evalArg.left(colonPos));
            if (currFilter== CosmeticFilter::MatchesCSSBefore)
                result.CallbackSubject.append(QStringLiteral(":before"));
            else if (currFilter == CosmeticFilter::MatchesCSSAfter)
                result.CallbackSubject.append(QStringLiteral(":after"));
            result.CallbackTarget = evalArg.mid(colonPos + cosmeticLen);
            result.CallbackTarget = result.CallbackTarget.left(result.CallbackTarget.indexOf(QChar(')')));
            return result;
        }
    }
    return result;
}

std::vector<std::tuple<int, AdBlockFilter::CosmeticFilter, int> > AdBlockFilter::getChainableFilters(const QString &evalStr) const
{
    // Only search for chainable types
    std::vector< std::tuple<int, CosmeticFilter, int> > filters;
    filters.push_back(std::make_tuple(evalStr.indexOf(QStringLiteral(":has-text(")), CosmeticFilter::HasText, 10));
    filters.push_back(std::make_tuple(evalStr.indexOf(QStringLiteral(":if(")), CosmeticFilter::If, 4));
    filters.push_back(std::make_tuple(evalStr.indexOf(QStringLiteral(":if-not(")), CosmeticFilter::IfNot, 8));
    filters.push_back(std::make_tuple(evalStr.indexOf(QStringLiteral(":matches-css(")), CosmeticFilter::MatchesCSS, 13));
    filters.push_back(std::make_tuple(evalStr.indexOf(QStringLiteral(":matches-css-before(")), CosmeticFilter::MatchesCSSBefore, 20));
    filters.push_back(std::make_tuple(evalStr.indexOf(QStringLiteral(":matches-css-after(")), CosmeticFilter::MatchesCSSAfter, 19));
    filters.push_back(std::make_tuple(evalStr.indexOf(QStringLiteral(":xpath(")), CosmeticFilter::XPath, 7));
    filters.erase(std::remove_if(filters.begin(), filters.end(), [](std::tuple<int, CosmeticFilter, int> p){ return std::get<0>(p) < 0; }), filters.end());
    if (filters.empty())
        return filters;

    std::sort(filters.begin(), filters.end(), [](std::tuple<int, CosmeticFilter, int> const &p1, std::tuple<int, CosmeticFilter, int> const &p2) {
        return std::get<0>(p1) < std::get<0>(p2);
    });

    return filters;
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
