#include "AdBlockFilterParser.h"
#include "AdBlockManager.h"
#include "Bitfield.h"
#include "CommonUtil.h"

#include <algorithm>
#include <array>
#include <QRegularExpression>
#include <QDebug>

namespace adblock
{

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
    { QStringLiteral("badfilter"), ElementType::BadFilter },           { QStringLiteral("inline-script"), ElementType::InlineScript },
    { QStringLiteral("3p"), ElementType::ThirdParty },                 { QStringLiteral("css"), ElementType::Stylesheet },
    { QStringLiteral("frame"), ElementType::Subdocument },             { QStringLiteral("xhr"), ElementType::XMLHTTPRequest },
    { QStringLiteral("doc"), ElementType::Document },                  { QStringLiteral("cname"), ElementType::NotImplemented },
    { QStringLiteral("popunder"), ElementType::NotImplemented },       { QStringLiteral("other"), ElementType::Other }
};

FilterParser::FilterParser(AdBlockManager *adBlockManager) :
    m_adBlockManager(adBlockManager)
{
}

std::unique_ptr<Filter> FilterParser::makeFilter(QString rule) const
{
    auto filter = std::make_unique<Filter>(rule);

    // Make sure filter is able to be parsed
    if (rule.isEmpty() || rule.startsWith(u'!'))
        return filter;

    Filter *filterPtr = filter.get();

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
    int pos = rule.indexOf(u'$');
    if (pos >= 0 && pos + 1 < rule.size() && rule.at(pos + 1).isLetter())
    {
        parseOptions(rule.mid(pos + 1), filterPtr);
        rule = rule.left(pos);
    }

    // Stop now if we can't support the filter yet
    if (filter->getCategory() == FilterCategory::NotImplemented)
        return filter;

    // Check if rule is a 'Match all' type
    if (rule.isEmpty() || (rule.size() == 1 && rule.at(0) == u'*'))
        filterPtr->m_matchAll = true;

    // Check if rule is a regular expression
    if (rule.size() > 1 && rule.startsWith(u'/') && rule.endsWith(u'/'))
    {
        filterPtr->m_category = FilterCategory::RegExp;

        rule = rule.mid(1);
        rule = rule.left(rule.size() - 1);

        QRegularExpression::PatternOptions options =
                (filterPtr->m_matchCase ? QRegularExpression::NoPatternOption : QRegularExpression::CaseInsensitiveOption);
        filterPtr->m_regExp = std::make_unique<QRegularExpression>(rule, options);
        return filter;
    }

    // Remove any leading wildcard
    if (rule.startsWith(u'*'))
        rule = rule.mid(1);

    // Remove trailing wildcard
    if (rule.endsWith(u'*'))
        rule = rule.left(rule.size() - 1);

    // Check for domain matching rule
    if (rule.startsWith(QStringLiteral("||")) && rule.endsWith(u'^') && isDomainRule(rule))
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

    // Check for blob: and data: filters
    parseForCSP(filterPtr);

    // If no category set by now, it is a string contains type
    if (filterPtr->getCategory() == FilterCategory::None)
    {
        filterPtr->m_category = FilterCategory::StringContains;

        // Pre-calculate hash of evaluation string for Rabin-Karp string matching
        filterPtr->hashEvalString();
    }

    return filter;
}

bool FilterParser::isDomainRule(const QString &rule) const
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

bool FilterParser::isStylesheetRule(const QString &rule, Filter *filter) const
{
    int pos = 0;

    // Ignore uBlock HTML filters and unsupported AdGuard directives
    //TODO: support #$# cosmetic filters (these, rather than adding "display: none!important;", include their own CSS rule(s)).
    //      Example: site.com#$#.someDiv { width: 1px!important; }
    const std::array<QString, 5> unsupportedTypes = { QStringLiteral("##^"), QStringLiteral("#%#"),
                                                    QStringLiteral("#@%#"), QStringLiteral("#$#"),
                                                    QStringLiteral("#@$#") };
    for (const QString &unsupported : unsupportedTypes)
    {
        if (rule.indexOf(unsupported) >= 0)
        {
            filter->m_category = FilterCategory::NotImplemented;
            return true;
        }
    }

    // Check if CSS rule
    const std::array<QString, 2> supportedTypes = { QStringLiteral("##"), QStringLiteral("#?#") };
    for (const QString &supportedRule : supportedTypes)
    {
        if ((pos = rule.indexOf(supportedRule)) >= 0)
        {
            filter->m_category = FilterCategory::Stylesheet;

            // Fill domain blacklist
            if (pos > 0)
                parseDomains(rule.left(pos), QChar(','), filter);

            filter->setEvalString(rule.mid(pos + supportedRule.size()));

            // Sanitize
            if (filter->getEvalString().contains(QStringLiteral("\"\\\"")))
            {
                QString sanitized = filter->getEvalString();
                sanitized.replace(QStringLiteral("\"\\\""), QStringLiteral("'\\\""))
                         .replace(QStringLiteral("\\\"\""), QStringLiteral("\\\"'"));
                filter->setEvalString(sanitized);
            }

            // Check for custom stylesheets, cosmetic options, and/or script injection filter rules
            if (parseCustomStylesheet(filter) || parseCosmeticOptions(filter) || parseScriptInjection(filter))
                return true;

            return true;
        }
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

bool FilterParser::parseCosmeticOptions(Filter *filter) const
{
    if (!filter->hasDomainRules())
        return false;

    // Replace -abp- terms with uBlock versions
    //filter->m_evalString.replace(QStringLiteral(":-abp-properties"), QStringLiteral(":has")); // too expensive to process this one
    filter->m_evalString.replace(QStringLiteral(":-abp-contains"), QStringLiteral(":has-text"));
    filter->m_evalString.replace(QStringLiteral(":-abp-has"), QStringLiteral(":if"));

    const static QString adGuardIfDirective = QStringLiteral("[-ext-has=");

    // Translate "[-ext-has=('|")....('|")]" into ":if(....)"
    int adGuardIndex = filter->m_evalString.indexOf(adGuardIfDirective);
    if (adGuardIndex >= 0)
    {
        const QChar quoteChar = filter->m_evalString.at(adGuardIndex + adGuardIfDirective.size());
        const int endIndex = filter->m_evalString.indexOf(quoteChar, adGuardIndex + adGuardIfDirective.size() + 1);
        filter->m_evalString.remove(endIndex + 1, 1);
        filter->m_evalString.replace(endIndex, 1, u')');
        filter->m_evalString.replace(adGuardIndex + adGuardIfDirective.size(), 1, u'(');
        filter->m_evalString.replace(adGuardIfDirective, QStringLiteral(":if"));
    }

    if (filter->m_evalString.indexOf(QStringLiteral(":-abp-")) >= 0)
    {
        filter->m_category = FilterCategory::None;
        return false;
    }

    // Workaround until a better approach towards procedural filter parsing/execution is implemented
    filter->m_evalString.replace(QStringLiteral(":not(:has("), QStringLiteral(":if-not("));

    // Search for each chainable type and handle the first one to appear, as any other
    // chainable filter options will appear as an argument of the first
    std::vector<ProceduralDirective> filters = getChainableDirectives(filter->m_evalString);
    if (filters.empty())
        return false;

    // Keep a copy of original evaluation string until processing is complete
    QString evalStr = filter->m_evalString;

    // Process first item in filters container
    const ProceduralDirective &directive = filters.at(0);

    // Extract filter argument
    QString evalArg = evalStr.mid(directive.Index + directive.StringLength);

    int argEndPos = 0;
    int numNestedArgs = 0;
    for (int i = 0; i < evalArg.size(); ++i)
    {
        const char currentChar = evalArg.at(i).toLatin1();
        if (currentChar == '(')
            ++numNestedArgs;
        else if (currentChar == ')')
        {
            if (numNestedArgs-- == 0)
            {
                argEndPos = i;
                break;
            }
        }
    }
    if (argEndPos <= 0)
        argEndPos = evalArg.lastIndexOf(QChar(')'));
    evalArg = evalArg.left(argEndPos);

    evalStr = evalStr.left(directive.Index);

    if (evalStr.isEmpty())
        evalStr = QStringLiteral("*");

    evalStr.replace(QChar('\''), QString("\\\'"));

    const bool isArgRegExp = evalArg.startsWith(QChar('/'))
            && (evalArg.endsWith(QChar('/'))
                || (evalArg.lastIndexOf(QChar('/')) + 3 >= evalArg.size()));
    if (!isArgRegExp || directive.DirectiveName == CosmeticFilter::XPath)
        evalArg.replace(QChar('\''), QString("\\\'"));

    switch (directive.DirectiveName)
    {
        case CosmeticFilter::HasText:
        {
            // Place argument in quotes if not a regular expression
            if (!isArgRegExp)
                evalArg = QString("'%1'").arg(evalArg);

            filter->m_evalString = QString("hideNodes(hasText, '%1', %2); ").arg(evalStr, evalArg);
            break;
        }
        case CosmeticFilter::Has:
        case CosmeticFilter::If:
        case CosmeticFilter::IfNot:
        case CosmeticFilter::Not:
        {
            const bool isNegation = (directive.DirectiveName == CosmeticFilter::IfNot || directive.DirectiveName == CosmeticFilter::Not);
            // Check if anything within the :if(...) is a cosmetic filter, thus requiring the use of callbacks
            if (filters.size() > 1)
            {
                CosmeticJSCallback c = getTranslation(evalArg, filters);
                if (c.IsValid)
                {
                    if (isNegation)
                        filter->m_evalString = QString("hideIfNotChain('%1', '%2', '%3', %4); ").arg(evalStr, c.CallbackSubject, c.CallbackTarget, c.CallbackName);
                    else
                        filter->m_evalString = QString("hideIfChain('%1', '%2', '%3', %4); ").arg(evalStr, c.CallbackSubject, c.CallbackTarget, c.CallbackName);
                    filter->m_category = FilterCategory::StylesheetJS;
                    return true;
                }
            }
            if (isNegation)
                filter->m_evalString = QString("hideIfNotHas('%1', '%2'); ").arg(evalStr, evalArg);
            else
                filter->m_evalString = QString("hideIfHas('%1', '%2'); ").arg(evalStr, evalArg);
            break;
        }
        case CosmeticFilter::MatchesCSS:
        {
            filter->m_evalString = QString("hideNodes(matchesCSS, '%1', '%2'); ").arg(evalStr, evalArg);
            break;
        }
        case CosmeticFilter::MatchesCSSBefore:
        {
            filter->m_evalString = QString("hideNodes(matchesCSSBefore, '%1', '%2'); ").arg(evalStr, evalArg);
            break;
        }
        case CosmeticFilter::MatchesCSSAfter:
        {
            filter->m_evalString = QString("hideNodes(matchesCSSAfter, '%1', '%2'); ").arg(evalStr, evalArg);
            break;
        }
        case CosmeticFilter::XPath:
        {
            filter->m_evalString = QString("hideNodes(doXPath, '%1', '%2'); ").arg(evalStr, evalArg);
            break;
        }
        case CosmeticFilter::NthAncestor:
        {
            filter->m_evalString = QString("hideNodes(nthAncestor, '%1', '%2'); ").arg(evalStr, evalArg);
            break;
        }
        case CosmeticFilter::MinTextLength:
        {
            filter->m_evalString = QString("hideNodes(minTextLength, '%1', %2); ").arg(evalStr, evalArg);
            break;
        }
        case CosmeticFilter::Upward:
        {
            filter->m_evalString = QString("hideNodes(upwardMatch, '%1', '%2'); ").arg(evalStr, evalArg);
            break;
        }
        case CosmeticFilter::Remove:
        {
            filter->m_evalString = QString("hideNodes(removeNodes, '%1', '%2'); ").arg(evalStr, evalArg);
            break;
        }
    }
    filter->m_category = FilterCategory::StylesheetJS;
    return true;
}

bool FilterParser::parseCustomStylesheet(Filter *filter) const
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

bool FilterParser::parseScriptInjection(Filter *filter) const
{
    // Check if filter is used for script injection, and has at least 1 domain option    
    if (!filter->hasDomainRules())
        return false;

    int keywordLength = 0;
    const std::array<QString, 2> scriptKeywords = { QStringLiteral("script:inject("), QStringLiteral("+js(") };
    for (const QString &keyword : scriptKeywords)
    {
        if (filter->m_evalString.startsWith(keyword))
        {
            keywordLength = keyword.size();
            break;
        }
    }

    if (keywordLength == 0)
        return false;

    filter->m_category = FilterCategory::Scriptlet;

    // Extract inner arguments and separate by ',' delimiter
    QString injectionStr = filter->m_evalString.mid(keywordLength);
    injectionStr = injectionStr.left(injectionStr.lastIndexOf(QChar(')')));

    QStringList injectionArgs = injectionStr.split(QChar(','), QStringSplitFlag::SkipEmptyParts);
    const QString &resourceName = injectionArgs.at(0);

    // Fetch resource from AdBlockManager and set value as m_evalString
    if (m_adBlockManager != nullptr)
    {
        filter->m_evalString = m_adBlockManager->getResource(resourceName);
        if (filter->m_evalString.isEmpty())
            return true;
    }

    const static QString nonThrowingScript = QStringLiteral("try { \n"
                                                 " %1 \n"
                                                 "} catch (ex) { \n"
                                                 "  console.error('[Viper Browser] Error running Advertisement Blocking script: ', ex); \n"
                                                 "  console.error(ex.stack); \n "
                                                 "} \n ");

    if (injectionArgs.size() < 2)
    {
        filter->m_evalString = nonThrowingScript.arg(filter->m_evalString);
        return true;
    }

    // For each item with index > 0 in injectionArgs list, replace the *first* {{index}}
    // string in resource with injectionArgs[index]
    for (int i = 1; i < injectionArgs.size(); ++i)
    {
        QString arg = injectionArgs.at(i).trimmed().replace(QChar('\''), QString("\\\'"));
        QString term = QString("{{%1}}").arg(i);

        const int targetIndex = filter->m_evalString.indexOf(term);
        if (targetIndex >= 0)
            filter->m_evalString.replace(targetIndex, term.size(), arg);

        //filter->m_evalString.replace(term, arg);
    }

    filter->m_evalString = nonThrowingScript.arg(filter->m_evalString);
    return true;
}

CosmeticJSCallback FilterParser::getTranslation(const QString &evalArg, const std::vector<ProceduralDirective> &filters) const
{
    auto result = CosmeticJSCallback();

    const ProceduralDirective &directive = filters.at(0);

    // Attempt to extract information neccessary for callback invocation
    int argStrLen = evalArg.size();
    for (std::size_t i = 1; i < filters.size(); ++i)
    {
        auto const &d2 = filters.at(i);
        if (d2.Index < directive.Index + directive.StringLength + argStrLen)
        {
            result.IsValid = true;

            // Extract selector and argument for nested filter
            int colonPos = d2.Index - directive.Index - directive.StringLength;
            int cosmeticLen = d2.StringLength;

            switch (d2.DirectiveName)
            {
                case CosmeticFilter::HasText:
                    result.CallbackName = QStringLiteral("hasText");
                    break;
                case CosmeticFilter::XPath:
                    result.CallbackName = QStringLiteral("doXPath");
                    break;
                case CosmeticFilter::MatchesCSS:
                    result.CallbackName = QStringLiteral("matchesCSS");
                    break;
                case CosmeticFilter::MatchesCSSBefore:
                    result.CallbackName = QStringLiteral("matchesCSSBefore");
                    break;
                case CosmeticFilter::MatchesCSSAfter:
                    result.CallbackName = QStringLiteral("matchesCSSAfter");
                    break;
                case CosmeticFilter::NthAncestor:
                    result.CallbackName = QStringLiteral("nthAncestor");
                    break;
                case CosmeticFilter::MinTextLength:
                    result.CallbackName = QStringLiteral("minTextLength");
                    break;
                case CosmeticFilter::Upward:
                    result.CallbackName = QStringLiteral("upwardMatch");
                    break;
                case CosmeticFilter::Remove:
                    result.CallbackName = QStringLiteral("removeNodes");
                    break;
                default:
                    // If, IfNot, and Has should not be nested (we really should support this however)
                    break;
            }

            result.CallbackSubject = QString("%1").arg(evalArg.left(colonPos));
            result.CallbackTarget = evalArg.mid(colonPos + cosmeticLen);
            result.CallbackTarget = result.CallbackTarget.left(result.CallbackTarget.indexOf(QChar(')')));
            return result;
        }
    }
    return result;
}

std::vector<ProceduralDirective> FilterParser::getChainableDirectives(const QString &evalStr) const
{
    // Only search for chainable types
    std::vector<ProceduralDirective> filters;
    auto lookForDirective = [&filters, &evalStr](const QString &directive, CosmeticFilter filterType) {
        const int directiveLen = directive.size();

        int lastIndex = 0;
        int dirIndex = evalStr.indexOf(directive);
        while (dirIndex >= 0)
        {
            // ProceduralDirective : { Index, StringLength, DirectiveName }
            filters.push_back({ dirIndex, directiveLen, filterType });

            lastIndex = dirIndex + directiveLen;
            dirIndex = evalStr.indexOf(directive, lastIndex);
        }
    };

    const std::array<std::pair<QString, CosmeticFilter>, 13> targets = {
        std::make_pair(QStringLiteral(":has("),                CosmeticFilter::Has),
        std::make_pair(QStringLiteral(":has-text("),           CosmeticFilter::HasText),
        std::make_pair(QStringLiteral(":if("),                 CosmeticFilter::If),
        std::make_pair(QStringLiteral(":if-not("),             CosmeticFilter::IfNot),
        std::make_pair(QStringLiteral(":not("),                CosmeticFilter::Not),
        std::make_pair(QStringLiteral(":matches-css("),        CosmeticFilter::MatchesCSS),
        std::make_pair(QStringLiteral(":matches-css-before("), CosmeticFilter::MatchesCSSBefore),
        std::make_pair(QStringLiteral(":matches-css-after("),  CosmeticFilter::MatchesCSSAfter),
        std::make_pair(QStringLiteral(":xpath("),              CosmeticFilter::XPath),
        std::make_pair(QStringLiteral(":nth-ancestor("),       CosmeticFilter::NthAncestor),
        std::make_pair(QStringLiteral(":min-text-length("),    CosmeticFilter::MinTextLength),
        std::make_pair(QStringLiteral(":upward("),             CosmeticFilter::Upward),
        std::make_pair(QStringLiteral(":remove("),             CosmeticFilter::Remove)
    };

    for (const std::pair<QString, CosmeticFilter> &target : targets)
        lookForDirective(target.first, target.second);

    if (filters.empty())
        return filters;

    std::sort(filters.begin(), filters.end(), [](const ProceduralDirective &d1, const ProceduralDirective &d2) {
        return d1.Index < d2.Index;
    });

    return filters;
}

void FilterParser::parseForCSP(Filter *filter) const
{
    std::vector<QString> cspDirectives;

    if (filter->m_evalString.startsWith(QLatin1String("blob:")))
    {
        filter->m_category = FilterCategory::Domain;
        filter->m_blockedTypes |= ElementType::CSP;
        filter->m_evalString = QString();

        if (filter->hasElementType(filter->m_blockedTypes, ElementType::Subdocument))
            cspDirectives.push_back(QLatin1String("frame-src 'self' * data:"));

        if (filter->hasElementType(filter->m_blockedTypes, ElementType::Script))
            cspDirectives.push_back(QLatin1String("script-src 'self' * data: 'unsafe-inline' 'unsafe-eval'"));

        if (cspDirectives.empty())
            cspDirectives.push_back(QLatin1String("default-src 'self' * data: 'unsafe-inline' 'unsafe-eval'"));

    }
    else if (filter->m_evalString.startsWith(QLatin1String("data:")))
    {
        filter->m_category = FilterCategory::Domain;
        filter->m_blockedTypes |= ElementType::CSP;
        filter->m_evalString = QString();

        if (filter->hasElementType(filter->m_blockedTypes, ElementType::Subdocument))
            cspDirectives.push_back(QLatin1String("frame-src 'self' * blob:"));

        if (filter->hasElementType(filter->m_blockedTypes, ElementType::Script))
            cspDirectives.push_back(QLatin1String("script-src 'self' * blob: 'unsafe-inline' 'unsafe-eval'"));

        if (cspDirectives.empty())
            cspDirectives.push_back(QLatin1String("default-src 'self' * blob: 'unsafe-inline' 'unsafe-eval'"));
    }

    if (!cspDirectives.empty())
    {
        QString cspConcatenated;
        for (size_t i = 0; i < cspDirectives.size(); ++i)
        {
            cspConcatenated.append(cspDirectives.at(i));
            if (i + 1 < cspDirectives.size())
                cspConcatenated.append(QLatin1String("; "));
        }

        filter->setContentSecurityPolicy(cspConcatenated);
    }
}

void FilterParser::parseDomains(const QString &domainString, QChar delimiter, Filter *filter) const
{
    QStringList domainList = domainString.split(delimiter, QStringSplitFlag::SkipEmptyParts);
	if (domainList.isEmpty())
		domainList.append(domainString);

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

void FilterParser::parseOptions(const QString &optionString, Filter *filter) const
{
    QStringList optionsList = optionString.split(QChar(','), QStringSplitFlag::SkipEmptyParts);
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
        //else if (option.startsWith(QStringLiteral("denyallow="))) - parse like domain option list, but,
        //                                                            different ruleset applies. this filter option means
        //                                                            to deny everything not listed in the denyallow list
        else if (option.startsWith(QStringLiteral("csp=")))
        {
            filter->m_blockedTypes |= ElementType::CSP;
            filter->setContentSecurityPolicy(option.mid(4));
        }
        // Handle options specific to uBlock Origin
        else if (option.startsWith(QStringLiteral("redirect=")))
        {
            filter->m_redirect = true;
            filter->m_redirectName = option.mid(9);
        }
        else if (option.startsWith(QStringLiteral("redirect-rule=")))
        {
            filter->m_redirect = true;
            filter->m_redirectName = option.mid(14);
        }
        else if (option.compare(QStringLiteral("empty")) == 0)
        {
            filter->m_redirect = true;
            filter->m_redirectName = QStringLiteral("nooptext");
        }
        else if (option.compare(QStringLiteral("mp4")) == 0)
        {
            filter->m_redirect = true;
            filter->m_redirectName = QStringLiteral("noopmp4-1s");
        }
        else if (option.compare(QStringLiteral("first-party")) == 0 || option.compare(QStringLiteral("1p")) == 0)
        {
            filter->m_allowedTypes |= ElementType::ThirdParty;
        }
        else if (option.compare(QStringLiteral("all")) == 0)
        {
            filter->m_blockedTypes |= static_cast<ElementType>(0x001047FFULL);
        }
        else if (!filter->m_exception && (option.compare(QStringLiteral("important")) == 0))
        {
            filter->m_important = true;
        }
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

    if (filter->hasElementType(filter->m_blockedTypes, ElementType::NotImplemented)
            || filter->hasElementType(filter->m_allowedTypes, ElementType::NotImplemented))
        filter->m_category = FilterCategory::NotImplemented;
}

QString FilterParser::parseRegExp(const QString &regExpString) const
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

}
