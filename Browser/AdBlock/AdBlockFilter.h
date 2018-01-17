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
    BadFilter        = 0x00040000,
    Other            = 0x00080000
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

/// Cosmetic filter types, used internally for creating appropriate JavaScript calls
/// has-text, if, if-not, matches-css, matches-css-before, matches-css-after, xpath
enum class CosmeticFilter
{
    Has,                 /// Selects the subject element(s) iff it contains at least one or more of the given targets
    HasText,             /// Selects the subject element(s) iff the given text is contained within the subject element
    If,                  /// Same as :has cosmetic filter, however this may take other procedural cosmetic filters as arguments, while :has cannot
    IfNot,               /// Selects the subject element(s) iff the result of evaluating the argument has exactly zero elements
    MatchesCSS,          /// Selects the subject element(s) iff the given argument (css) is a subset of the subject's CSS properties
    MatchesCSSBefore,    /// Same as :matches-css cosmetic filter, with the :before pseudo-class attached to the subject
    MatchesCSSAfter,     /// Same as :matches-css cosmetic filter, with the :after pseudo-class attached to the subject
    XPath                /// Creates a new set of elements by evaluating a XPath. Uses an optional subject as the context node and the argument as the expression.
};

/**
 * @class AdBlockFilter
 * @brief An implementation of an AdBlock Plus filter for network content
 */
class AdBlockFilter
{
    friend class AdBlockFilterParser;
    friend class AdBlockManager;

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
    /// Constructs the filter given the corresponding rule (a line in an adblock plus-formatted file)
    explicit AdBlockFilter(const QString &rule);

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

    /// Returns true if the given ElementType bitfield is set for the bit associated with the target ElementType
    inline bool hasElementType(ElementType subject, ElementType target) const
    {
        return (subject & target) == target;
    }

protected:
    /// Adds the given domain to the whitelist
    void addDomainToWhitelist(const QString &domainStr);

    /// Adds the given domain to the blacklist
    void addDomainToBlacklist(const QString &domainStr);

    /// Sets the evaluation string used to match network requests
    void setEvalString(const QString &evalString);

    /// Evaluates the rule, setting the filter to reflect the corresponding value(s)
    void setRule(const QString &rule);

    /// Calculates the hash value of the evaluation string and the difference hash variable, used in Rabin-Karp string matching algorithm
    void hashEvalString();

private:
    /// Returns true if the given domain matches the base domain string, false if else
    bool isDomainMatch(QString base, const QString &domainStr) const;

    /// Compares the requested URL and domain to the evaluation string, returning true if the filter matches the request, false if else
    bool isDomainStartMatch(const QString &requestUrl, const QString &secondLevelDomain) const;

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

private:
    /// Used for string hash computations in rabin-karp matching algorithm
    quint64 m_differenceHash;

    /// Contains a hash of the evaluation string, used if filter category is StringContains
    size_t m_evalStringHash;
};

#endif // ADBLOCKFILTER_H
