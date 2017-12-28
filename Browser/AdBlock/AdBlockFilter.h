#ifndef ADBLOCKFILTER_H
#define ADBLOCKFILTER_H

#include "Bitfield.h"
#include <cstdint>
#include <memory>
#include <tuple>
#include <QHash>
#include <QRegularExpression>
#include <QSet>
#include <QString>

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
template<>
struct EnableBitfield<ElementType>
{
    static constexpr bool enabled = true;
};

/// Mapping of option name strings to their corresponding \ref ElementType
extern QHash<QString, ElementType> eOptionMap;

/// Categories that an AdBlock filter may belong to. A filter may only belong to one category
enum class FilterCategory
{
    None,
    Stylesheet,          /// Block or allow CSS elements
    StylesheetJS,        /// Block or allow CSS elements via JavaScript injection
    StylesheetCustom,    /// Custom CSS for filter element as per :style option
    Domain,              /// Block or allow a domain
    DomainStart,         /// Block or allow based on domain starting with given filter expression
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
    friend class AdBlockManager;

    /// Cosmetic filter types, used internally for creating appropriate JavaScript calls
    /// has-text, if, if-not, matches-css, matches-css-before, matches-css-after, xpath
    enum class CosmeticFilter
    {
        Has,
        HasText,
        If,
        IfNot,
        MatchesCSS,
        MatchesCSSBefore,
        MatchesCSSAfter,
        XPath
    };

    /// Contains data necessary for translating uBlock cosmetic filter rules into the appropriate JavaScript calls
    struct CosmeticJSCallback
    {
        /// True if the callback information is valid, false if this structure contains no useful information
        bool IsValid;

        /// Name of the callback to be invoked, or empty if IsNested is false
        QString CallbackName;

        /// Subject to be passed to the callback if
        QString CallbackSubject;

        /// Target to be searched for in the callback
        QString CallbackTarget;
    };

    /// Returns true if the given ElementType bitfield is set for the bit associated with the target ElementType
    inline bool hasElementType(ElementType subject, ElementType target) const
    {
        return (subject & target) == target;
    }

    /// Computes and returns base^exp
    inline quint64 quPow(quint64 base, quint64 exp) const
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

public:
    /// Default constructor
    AdBlockFilter();

    /// Constructs the filter given the corresponding rule (a line in an adblock plus-formatted file)
    AdBlockFilter(const QString &rule);

    /// Copy constructor
    AdBlockFilter(const AdBlockFilter &other);

    /// Move constructor
    AdBlockFilter(AdBlockFilter &&other);

    /// Copy assignment operator
    AdBlockFilter &operator =(const AdBlockFilter &other);

    /// Move assignment operator
    AdBlockFilter &operator =(AdBlockFilter &&other);

    /// Destructor
    ~AdBlockFilter();

    /// Returns the category of this filter
    FilterCategory getCategory() const;

    /// Evaluates the rule, setting the filter to reflect the corresponding value(s)
    void setRule(const QString &rule);

    /// Returns the original filter rule as a QString
    const QString &getRule() const;

    /// Returns the evaluation string of the rule
    const QString &getEvalString() const;

    /// Returns true if the filter is an exception, false if it is a standard blocking filter
    bool isException() const;

    /// Returns true if the filter is a blocking type and has the important option specified, false if else
    bool isImportant() const;

    /// Returns true if there are domain-specific settings on the filter, false if else
    bool hasDomainRules() const;

    /**
     * @brief Determines whether or not the network request matches the filter
     * @param baseUrl URL of the original network request
     * @param requestUrl URL of the actual network request
     * @param requestDomain Domain of the request URL
     * @param typeMask Element type(s) associated with the request. Filter will disregard if type is set to none
     * @return True if request matches filter, false if else.
     */
    bool isMatch(const QString &baseUrl, const QString &requestUrl, const QString &requestDomain, ElementType typeMask);

    /// Returns true if this rule is of the Stylesheet category and applies to the given domain, returns false if else.
    bool isDomainStyleMatch(const QString &domain);

protected:
    /// Adds the given domain to the whitelist
    void addDomainToWhitelist(const QString &domainStr);

private:
    /// Returns true if the given domain matches the base domain string, false if else
    bool isDomainMatch(QString base, const QString &domainStr) const;

    /// Compares the requested URL and domain to the evaluation string, returning true if the filter matches the request, false if else
    bool isDomainStartMatch(const QString &requestUrl, const QString &secondLevelDomain) const;

    /// Parses the rule string, setting the appropriate fields of the filter object
    void parseRule();

    /// Returns true if, while parsing the filter rule, its category is determined to be of type Stylesheet or StylesheetJS. Otherwise returns false.
    bool isStylesheetRule();

    /// Parses a list of domains, separated with the given delimiter, and placing them into
    /// either the domain blacklist or whitelist depending on the syntax (~ = whitelist, default = blacklist)
    void parseDomains(const QString &domainString, QChar delimiter);

    /// Parses a comma separated list of options contained within the given string
    void parseOptions(const QString &optionString);

    /// Parses the rule string for uBlock Origin style cosmetic filter options, returning true if category is StylesheetJS, false if else
    bool parseCosmeticOptions();

    /// Handles the :style option for stylesheet filters, returning true if category is StylesheetCustom, false if else
    bool parseCustomStylesheet();

    /// Checks for and handles the script:inject(...) filter option, returning true if found, false if else
    bool parseScriptInjection();

    /// Returns the javascript callback translation structure for the given evaluation argument and a container of index-type-string len filter information pairs
    CosmeticJSCallback getTranslation(const QString &evalArg, const std::vector<std::tuple<int, CosmeticFilter, int>> &filters);

    /// Returns a container of tuples including the index, type, and string length of each chainable cosmetic filter in the evaluation string
    std::vector< std::tuple<int, CosmeticFilter, int> > getChainableFilters(const QString &evalStr) const;

    /// Parses the given ad block plus formatted regular expression, returning the equivalent for a QRegularExpression
    QString parseRegExp(const QString &regExpString);

    /// Applies the Rabin-Karp string matching algorithm to the given string, returning true if it contains the filter's eval string, false if else
    /// From: https://www.joelverhagen.com/blog/2011/11/three-string-matching-algorithms-in-c/
    bool filterContains(const QString &haystack) const;

protected:
    /// Filter category
    FilterCategory m_category;

    /// Original rule string
    QString m_ruleString;

    /// Comparison string for evaluating rules (only used if category of filter is not RegExp)
    QString m_evalString;

    /// True if the filter is an exception, false if it is a standard blocking rule
    bool m_exception;

    /// True if the filter has the important option and is not an exception (uBlock standard)
    bool m_important;

    /// True if filter is disabled (will never match network requests), false if enabled (default)
    bool m_disabled;

    /// Bitfield of element types to be allowed
    ElementType m_allowedTypes;

    /// Bitfield of element types to be filtered
    ElementType m_blockedTypes;

    /// If true, the filter only applies to addresses with a matching letter case
    bool m_matchCase;

    /// Set to true if evaluation string is empty. Requests will still be checked based on domain blacklist/whitelist and allowed/blocked element types, etc
    bool m_matchAll;

    /// List of domains that the filter rule applies to. Specified by the domain filter option
    QSet<QString> m_domainBlacklist;

    /// List of domains that the filter rule does not apply to. Specified by the domain filter option
    QSet<QString> m_domainWhitelist;

    /// Unique pointer to a regular expression used by the filter, if filter is of the category RegExp
    std::unique_ptr<QRegularExpression> m_regExp;
};

#endif // ADBLOCKFILTER_H
