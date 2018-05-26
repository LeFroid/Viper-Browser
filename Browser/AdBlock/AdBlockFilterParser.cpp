#include "AdBlockFilterParser.h"
#include "AdBlockManager.h"
#include "Bitfield.h"

#include <algorithm>
#include <QRegularExpression>

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

std::unique_ptr<AdBlockFilter> AdBlockFilterParser::makeFilter(QString rule) const
{
    auto filter = std::make_unique<AdBlockFilter>(rule);

    // Make sure filter is able to be parsed
    if (rule.isEmpty() || rule.startsWith('!'))
        return filter;

    AdBlockFilter *filterPtr = filter.get();

    // Check if CSS rule
    if (isStylesheetRule(rule, filterPtr))
        return filter;

    // Check if the rule is an exception
    if (rule.startsWith(QStringLiteral("@@")))
    {
        filterPtr->m_exception = true;
        rule = rule.mid(2);
    }

    // Check if filter options are set
    int pos = rule.indexOf(QChar('$'));
    if (pos >= 0 && rule.at(rule.size() - 1).isLetter())
    {
        parseOptions(rule.mid(pos + 1), filterPtr);
        rule = rule.left(pos);
    }

    // Check if rule is a regular expression
    if (rule.startsWith('/') && rule.endsWith('/'))
    {
        filterPtr->m_category = FilterCategory::RegExp;

        rule = rule.mid(1);
        rule = rule.left(rule.size() - 1);

        QRegularExpression::PatternOptions options =
                (filterPtr->m_matchCase ? QRegularExpression::NoPatternOption : QRegularExpression::CaseInsensitiveOption);
        filterPtr->m_regExp = std::make_unique<QRegularExpression>(rule.replace("\\", "\\\\"), options);
        return filter;
    }

    // Remove any leading wildcard
    if (rule.startsWith('*'))
        rule = rule.mid(1);

    // Remove trailing wildcard
    if (rule.endsWith('*'))
        rule = rule.left(rule.size() - 1);

    // Check for domain matching rule
    if (rule.startsWith(QStringLiteral("||")) && rule.endsWith('^') && isDomainRule(rule))
    {
        rule = rule.mid(2);
        filterPtr->m_evalString = rule.left(rule.size() - 1);
        filterPtr->m_category = FilterCategory::Domain;
        return filter;
    }

    // Check if a regular expression might be needed
    const bool maybeRegExp = rule.contains('*') || rule.contains('^');

    // Domain start match
    if (rule.startsWith(QStringLiteral("||")) && !maybeRegExp)
    {
        filterPtr->m_category = FilterCategory::DomainStart;
        filterPtr->m_evalString = rule.mid(2);

        if (!filterPtr->m_matchCase)
            filterPtr->m_evalString = filterPtr->m_evalString.toLower();
        return filter;
    }

    // String start match
    if (rule.startsWith('|') && rule.at(1) != '|' && !maybeRegExp)
    {
        filterPtr->m_category = FilterCategory::StringStartMatch;
        rule = rule.mid(1);
    }

    // String end match (or exact match)
    if (rule.endsWith('|') && !maybeRegExp)
    {
        if (filterPtr->getCategory() == FilterCategory::StringStartMatch)
            filterPtr->m_category = FilterCategory::StringExactMatch;
        else
            filterPtr->m_category = FilterCategory::StringEndMatch;

        rule = rule.left(rule.size() - 1);
    }

    // Ad block format -> regular expression conversion
    if (maybeRegExp || rule.contains(QChar('|')))
    {
        QRegularExpression::PatternOptions options =
                (filterPtr->m_matchCase ? QRegularExpression::NoPatternOption : QRegularExpression::CaseInsensitiveOption);
        filterPtr->m_regExp = std::make_unique<QRegularExpression>(parseRegExp(rule), options);
        filterPtr->m_category = FilterCategory::RegExp;
        return filter;
    }

    // Set evaluation string based on the processed rule string
    filterPtr->setEvalString(rule);

    if (filterPtr->m_evalString.isEmpty())
        filterPtr->m_matchAll = true;

    if (!filterPtr->m_matchCase)
        filterPtr->m_evalString = filterPtr->m_evalString.toLower();

    // If no category set by now, it is a string contains type
    if (filterPtr->getCategory() == FilterCategory::None)
    {
        filterPtr->m_category = FilterCategory::StringContains;

        // Pre-calculate hash of evaluation string for Rabin-Karp string matching
        filterPtr->hashEvalString();
    }

    return filter;
}

bool AdBlockFilterParser::isDomainRule(const QString &rule) const
{
    // looping through string of format: "||inner_rule_text^", indices 0,1, and len-1 ignored
    for (int i = 2; i < rule.size() - 1; ++i)
    {
        switch (rule.at(i).toLatin1())
        {
            case '/':
            case ':':
            case '?':
            case '=':
            case '&':
            case '*':
                return false;
            default:
                break;
        }
    }

    return true;
}

bool AdBlockFilterParser::isStylesheetRule(const QString &rule, AdBlockFilter *filter) const
{
    int pos = 0;

    // Check if CSS rule
    if ((pos = rule.indexOf(QStringLiteral("##"))) >= 0)
    {
        filter->m_category = FilterCategory::Stylesheet;

        // Fill domain blacklist
        if (pos > 0)
            parseDomains(rule.left(pos), QChar(','), filter);

        filter->setEvalString(rule.mid(pos + 2));

        // Check for custom stylesheets, cosmetic options, and/or script injection filter rules
        if (parseCustomStylesheet(filter) || parseCosmeticOptions(filter) || parseScriptInjection(filter))
            return true;

        return true;
    }

    // Check if CSS exception
    if ((pos = rule.indexOf(QStringLiteral("#@#"))) >= 0)
    {
        filter->m_category = FilterCategory::Stylesheet;
        filter->m_exception = true;

        // Fill domain whitelist
        if (pos > 0)
            parseDomains(rule.left(pos), QChar(','), filter);

        filter->setEvalString(rule.mid(pos + 3));

        return true;
    }

    return false;
}

bool AdBlockFilterParser::parseCosmeticOptions(AdBlockFilter *filter) const
{
    if (!filter->hasDomainRules())
        return false;

    // Replace -abp- terms with uBlock versions
    filter->m_evalString.replace(QStringLiteral(":-abp-contains"), QStringLiteral(":has-text"));
    filter->m_evalString.replace(QStringLiteral(":-abp-has"), QStringLiteral(":if"));

    // Check for :has(..)
    int hasIdx = filter->m_evalString.indexOf(QStringLiteral(":has("));
    if (hasIdx >= 0)
    {
        QString hasArg = filter->m_evalString.mid(hasIdx + 5);
        hasArg = hasArg.left(hasArg.size() - 1);

        filter->m_evalString = filter->m_evalString.left(hasIdx);
        if (filter->m_evalString.isEmpty())
            return false;

        filter->m_evalString = QString("hideIfHas('%1', '%2'); ").arg(filter->m_evalString).arg(hasArg);
        filter->m_category = FilterCategory::StylesheetJS;

        // :has cannot be chained, so exit the method at this point
        return true;
    }

    // Search for each chainable type and handle the first one to appear, as any other
    // chainable filter options will appear as an argument of the first
    std::vector<std::tuple<int, CosmeticFilter, int>> filters = getChainableFilters(filter->m_evalString);
    if (filters.empty())
        return false;

    // Keep a copy of original evaluation string until processing is complete
    QString evalStr = filter->m_evalString;

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

            filter->m_evalString = QString("hideNodes(hasText, '%1', %2); ").arg(evalStr).arg(evalArg);
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
                        filter->m_evalString = QString("hideIfNotChain('%1', '%2', '%3', %4); ").arg(evalStr).arg(c.CallbackSubject).arg(c.CallbackTarget).arg(c.CallbackName);
                    else
                        filter->m_evalString = QString("hideIfChain('%1', '%2', '%3', %4); ").arg(evalStr).arg(c.CallbackSubject).arg(c.CallbackTarget).arg(c.CallbackName);
                    filter->m_category = FilterCategory::StylesheetJS;
                    return true;
                }
            }
            if (isNegation)
                filter->m_evalString = QString("hideIfNotHas('%1', '%2'); ").arg(evalStr).arg(evalArg);
            else
                filter->m_evalString = QString("hideIfHas('%1', '%2'); ").arg(evalStr).arg(evalArg);
            break;
        }
        case CosmeticFilter::MatchesCSS:
        {
            filter->m_evalString = QString("hideNodes(matchesCSS, '%1', '%2'); ").arg(evalStr).arg(evalArg);
            break;
        }
        case CosmeticFilter::MatchesCSSBefore:
        {
            evalStr.append(QStringLiteral(":before"));
            filter->m_evalString = QString("hideNodes(matchesCSS, '%1', '%2'); ").arg(evalStr).arg(evalArg);
            break;
        }
        case CosmeticFilter::MatchesCSSAfter:
        {
            evalStr.append(QStringLiteral(":after"));
            filter->m_evalString = QString("hideNodes(matchesCSS, '%1', '%2'); ").arg(evalStr).arg(evalArg);
            break;
        }
        case CosmeticFilter::XPath:
        {
            filter->m_evalString = QString("hideNodes(doXPath, '%1', '%2'); ").arg(evalStr).arg(evalArg);
            break;
        }
        default: return false;
    }
    filter->m_category = FilterCategory::StylesheetJS;
    return true;
}

bool AdBlockFilterParser::parseCustomStylesheet(AdBlockFilter *filter) const
{
    int styleIdx = filter->m_evalString.indexOf(QStringLiteral(":style("));
    if (styleIdx < 0)
        return false;

    // Extract style, and convert evaluation string into format "selector { style }"
    QString style = filter->m_evalString.mid(styleIdx + 7);
    style = style.left(style.lastIndexOf(QChar(')')));

    filter->m_evalString = filter->m_evalString.left(styleIdx).append(QString(" { %1 } ").arg(style));
    filter->m_category = FilterCategory::StylesheetCustom;

    return true;
}

bool AdBlockFilterParser::parseScriptInjection(AdBlockFilter *filter) const
{
    // Check if filter is used for script injection, and has at least 1 domain option
    if (!filter->m_evalString.startsWith(QStringLiteral("script:inject(")) || !filter->hasDomainRules())
        return false;

    filter->m_category = FilterCategory::StylesheetJS;

    // Extract inner arguments and separate by ',' delimiter
    QString injectionStr = filter->m_evalString.mid(14);
    injectionStr = injectionStr.left(injectionStr.lastIndexOf(QChar(')')));

    QStringList injectionArgs = injectionStr.split(QChar(','), QString::SkipEmptyParts);
    const QString &resourceName = injectionArgs.at(0);

    // Fetch resource from AdBlockManager and set value as m_evalString
    filter->m_evalString = AdBlockManager::instance().getResource(resourceName);
    if (injectionArgs.size() < 2)
        return true;

    // For each item with index > 0 in injectionArgs list, replace each {{index}}
    // string in resource with injectionArgs[index]
    for (int i = 1; i < injectionArgs.size(); ++i)
    {
        QString arg = injectionArgs.at(i).trimmed();
        arg.replace(QChar('\''), QStringLiteral("\\'"));
        QString term = QString("{{%1}}").arg(i);
        filter->m_evalString.replace(term, arg);
    }

    return true;
}

CosmeticJSCallback AdBlockFilterParser::getTranslation(const QString &evalArg, const std::vector<std::tuple<int, CosmeticFilter, int>> &filters) const
{
    auto result = CosmeticJSCallback();

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
            if (currFilter == CosmeticFilter::MatchesCSSBefore)
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

std::vector< std::tuple<int, CosmeticFilter, int> > AdBlockFilterParser::getChainableFilters(const QString &evalStr) const
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
void AdBlockFilterParser::parseDomains(const QString &domainString, QChar delimiter, AdBlockFilter *filter) const
{
    QStringList domainList = domainString.split(delimiter, QString::SkipEmptyParts);
    for (QString &domain : domainList)
    {
        // Check if domain is an entity filter type
        if (domain.endsWith(QStringLiteral(".*")))
            domain = domain.left(domain.size() - 1);

        if (domain.at(0) == QChar('~'))
            filter->addDomainToWhitelist(domain.mid(1));
        else
            filter->addDomainToBlacklist(domain);
    }
}

void AdBlockFilterParser::parseOptions(const QString &optionString, AdBlockFilter *filter) const
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
                filter->m_matchCase = true;

            if (optionException)
                filter->m_allowedTypes |= elemType;
            else
            {
                // Don't allow exception filters to whitelist entire pages / disable ad block (uBlock origin setting)
                if (filter->m_exception && elemType == ElementType::Document)
                    filter->m_disabled = true;

                filter->m_blockedTypes |= elemType;
            }
        }
        else if (option.startsWith(QStringLiteral("domain=")))
        {
            parseDomains(option.mid(7), QChar('|'), filter);
        }
        // Handle options specific to uBlock Origin
        else if (option.compare(QStringLiteral("first-party")) == 0)
            filter->m_blockedTypes |= ElementType::ThirdParty;
        else if (!filter->m_exception && (option.compare(QStringLiteral("important")) == 0))
            filter->m_important = true;
    }

    // Check if BadFilter option was set, and remove the badfilter string from original rule
    // (so the hash can be made and compared to other filters for removal)
    if (filter->hasElementType(filter->m_blockedTypes, ElementType::BadFilter))
    {
        if (filter->m_ruleString.endsWith(QStringLiteral(",badfilter")))
            filter->m_ruleString = filter->m_ruleString.left(filter->m_ruleString.indexOf(QStringLiteral(",badfilter")));
        else if (filter->m_ruleString.endsWith(QStringLiteral("$badfilter")))
            filter->m_ruleString = filter->m_ruleString.left(filter->m_ruleString.indexOf(QStringLiteral("$badfilter")));
    }
}

QString AdBlockFilterParser::parseRegExp(const QString &regExpString) const
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
