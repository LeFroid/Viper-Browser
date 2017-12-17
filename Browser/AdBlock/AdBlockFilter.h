#ifndef ADBLOCKFILTER_H
#define ADBLOCKFILTER_H

#include <cstdint>
#include <memory>
#include <QHash>
#include <QRegularExpression>
#include <QSet>
#include <QString>

namespace AdBlock
{
    /// Specific types of elements that can be filtered or whitelisted
    enum class ElementType : uint32_t
    {
        None             = 0x00000000,
        Script           = 0x00000001,
        Image            = 0x00000002,
        Stylesheet       = 0x00000004,
        Object           = 0x00000008,
        XMLHTTPRequest   = 0x00000010,
        ObjectSubrequest = 0x00000020,
        Subdocument      = 0x00000040,
        Ping             = 0x00000080,
        WebSocket        = 0x00000100,
        WebRTC           = 0x00000200,
        Document         = 0x00000400,
        ElemHide         = 0x00000800,
        GenericHide      = 0x00001000,
        GenericBlock     = 0x00002000,
        PopUp            = 0x00004000,
        ThirdParty       = 0x00008000,
        MatchCase        = 0x00010000,
        Collapse         = 0x00020000,
        Other            = 0x00040000
    };

    /// Mapping of option name strings to their corresponding \ref ElementType
    extern QHash<QString, ElementType> eOptionMap;

    /// Categories that an AdBlock filter may belong to. A filter may only belong to one category
    enum class FilterCategory
    {
        None,
        Stylesheet,          /// Block or allow CSS elements
        Domain,              /// Block or allow a domain
        StringStartMatch,    /// Block or allow based on strings starting with the filter expression
        StringEndMatch,      /// Block or allow based on strings ending with the filter expression
        StringExactMatch,    /// Block or allow if request has an exact match
        StringContains,      /// Block or allow if request contains the string in this filter
        RegExp               /// Block or allow based on a regular expression
    };

    /**
     * @class AdBlockFilter
     * @brief An implementation of an AdBlock Plus filter for network content
     */
    class AdBlockFilter
    {
        /// Default constructor
        AdBlockFilter();

        /// Constructs the filter given the corresponding rule (a line in an adblock plus-formatted file)
        AdBlockFilter(const QString &rule);

        /// Returns the category of this filter
        FilterCategory getCategory() const;

        /// Evaluates the rule, setting the filter to reflect the corresponding value(s)
        void setRule(const QString &rule);

        /// Returns the original filter rule as a QString
        const QString &getRule() const;

    private:
        /// Parses the rule string, setting the appropriate fields of the filter object
        void parseRule();

        /// Parses a list of domains, separated with the given delimiter, and placing them into
        /// either the domain blacklist or whitelist depending on the syntax (~ = whitelist, default = blacklist)
        void parseDomains(const QString &domainString, QChar delimiter);

        /// Parses a comma separated list of options contained within the given string
        void parseOptions(const QString &optionString);

        /// Parses the given ad block plus formatted regular expression, returning the equivalent for a QRegularExpression
        QString parseRegExp(const QString &regExpString);

    private:
        /// Filter category
        FilterCategory m_category;

        /// Original rule string
        QString m_ruleString;

        /// Comparison string for evaluating rules (only used if category of filter is not RegExp)
        QString m_evalString;

        /// True if the filter is an exception, false if it is a standard blocking rule
        bool m_exception;

        /// Bitfield of element types to be allowed
        ElementType m_allowedTypes;

        /// Bitfield of element types to be filtered
        ElementType m_blockedTypes;

        /// If true, the filter only applies to addresses with a matching letter case
        bool m_matchCase;

        /// If true, the rule applies to requests from a different origin than the page being loaded
        bool m_thirdParty;

        /// List of domains that the filter rule applies to. Specified by the domain filter option
        QSet<QString> m_domainBlacklist;

        /// List of domains that the filter rule does not apply to. Specified by the domain filter option
        QSet<QString> m_domainWhitelist;

        /// Unique pointer to a regular expression used by the filter, if filter is of the category RegExp
        std::unique_ptr<QRegularExpression> m_regExp;
    };
}

#endif // ADBLOCKFILTER_H
