#include "AdBlockFilter.h"
#include "Bitfield.h"

namespace AdBlock
{
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
        m_allowedTypes(ElementType::None),
        m_blockedTypes(ElementType::None),
        m_matchCase(false),
        m_thirdParty(false),
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
        m_allowedTypes(ElementType::None),
        m_blockedTypes(ElementType::None),
        m_matchCase(false),
        m_thirdParty(false),
        m_domainBlacklist(),
        m_domainWhitelist(),
        m_regExp(nullptr)
    {
        // Only bother parsing the rule if the given string is not empty and is not a comment
        if (!rule.isEmpty() && !rule.startsWith('!'))
            parseRule();
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
            m_thirdParty = false;
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
            m_regExp = std::make_unique<QRegularExpression>(rule, options);
            return;
        }

        // Remove any leading and/or trailing wildcards
        if (rule.startsWith('*'))
            rule = rule.mid(1);
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

        // Check if a regular expression is needed
        if (rule.contains('*') || rule.contains('^'))
        {
            QRegularExpression::PatternOptions options =
                    (m_matchCase ? QRegularExpression::NoPatternOption : QRegularExpression::CaseInsensitiveOption);
            m_regExp = std::make_unique<QRegularExpression>(parseRegExp(rule), options);
            m_category = FilterCategory::RegExp;
            return;
        }

        // String start match
        if (rule.startsWith('|') && rule.at(1) != '|')
        {
            m_category = FilterCategory::StringStartMatch;
            rule = rule.mid(1);
        }

        // String end match (or exact match)
        if (rule.endsWith('|'))
        {
            if (m_category == FilterCategory::StringStartMatch)
                m_category = FilterCategory::StringExactMatch;
            else
                m_category = FilterCategory::StringEndMatch;

            rule = rule.left(rule.size() - 1);
        }

        // If no category set by now, it is a string contains type
        if (m_category == FilterCategory::None)
            m_category = FilterCategory::StringContains;

        m_evalString = rule;
    }

    void AdBlockFilter::parseDomains(const QString &domainString, QChar delimiter)
    {
        QStringList domainList = domainString.split(delimiter, QString::SkipEmptyParts);
        for (const QString &domain : domainList)
        {
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
                const ElementType &elemType = it.value();
                if (elemType == ElementType::MatchCase)
                    m_matchCase = true;

                if (optionException)
                    m_allowedTypes = m_allowedTypes | it.value();
                else
                    m_blockedTypes = m_blockedTypes | it.value();
            }
            else if (option.startsWith("domain="))
            {
                parseDomains(option.mid(7), QChar('|'));
            }
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
                        replacement.append(".*");
                        usedStar = true;
                    }
                    break;
                }
                case '^':
                {
                    replacement.append("(?:[^\\w\\d\\-.%]|$)");
                    break;
                }
                case '|':
                {
                    if (i == 0)
                    {
                        if (regExpString.at(1) == '|')
                            replacement.append("^([\\w\\-]+:\\/+)?(?!\\/)(?:[^\\/]+\\.)?");
                        else
                            replacement.append('^');
                    }
                    else if (i == strSize - 1)
                        replacement.append('$');

                    break;
                }
                default:
                {
                    if (c.isLetterOrNumber() || c.isMark() || c == '_')
                        replacement.append('\\' + c);
                    else
                        replacement.append(c);

                    break;
                }
            }
        }
        return replacement;
    }
}
