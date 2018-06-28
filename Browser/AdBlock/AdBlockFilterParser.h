#ifndef ADBLOCKFILTERPARSER_H
#define ADBLOCKFILTERPARSER_H

#include "AdBlockFilter.h"

#include <memory>
#include <QString>

/// Mapping of option name strings to their corresponding \ref ElementType
extern QHash<QString, ElementType> eOptionMap;

/**
 * @ingroup AdBlock
 * @brief Contains data necessary for translating uBlock cosmetic filter rules into the appropriate JavaScript calls
 */
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

    /// Default constructor
    CosmeticJSCallback() : IsValid(false), CallbackName(), CallbackSubject(), CallbackTarget() {}
};

/**
 * @class AdBlockFilterParser
 * @ingroup AdBlock
 * @brief Instantiates \ref AdBlockFilter objects from rule strings formatted
 *        as per the AdBlock Plus or uBlock Origin specifications
 */
class AdBlockFilterParser
{
public:
    /// Default constructor
    AdBlockFilterParser() = default;

    /// Instantiates and returns an AdBlockFilter given a filter string
    std::unique_ptr<AdBlockFilter> makeFilter(QString rule) const;

private:
    /// Returns true if the given rule string is able to be interpreted as a domain anchor rule with no regular expressions.
    /// Example [will return true]: ||my.adserver.com^
    /// Example [will return false]: ||ads.*.host.com^
    bool isDomainRule(const QString &rule) const;

    /// Returns true if, while parsing the filter rule, its category is determined to be of type Stylesheet or StylesheetJS. Otherwise returns false.
    bool isStylesheetRule(const QString &rule, AdBlockFilter *filter) const;

    /// Parses the rule string for uBlock Origin style cosmetic filter options, returning true if category is StylesheetJS, false if else
    bool parseCosmeticOptions(AdBlockFilter *filter) const;

    /// Handles the :style option for stylesheet filters, returning true if category is StylesheetCustom, false if else
    bool parseCustomStylesheet(AdBlockFilter *filter) const;

    /// Checks for and handles the script:inject(...) filter option, returning true if found, false if else
    bool parseScriptInjection(AdBlockFilter *filter) const;

    /// Returns the javascript callback translation structure for the given evaluation argument and a container of index-type-string len filter information pairs
    CosmeticJSCallback getTranslation(const QString &evalArg, const std::vector<std::tuple<int, CosmeticFilter, int>> &filters) const;

    /// Returns a container of tuples including the index, type, and string length of each chainable cosmetic filter in the evaluation string
    std::vector< std::tuple<int, CosmeticFilter, int> > getChainableFilters(const QString &evalStr) const;

    /// Checks for blob: and data: type filter rules, converting them into the appropriate CSP filter types
    void parseForCSP(AdBlockFilter *filter) const;

    /// Parses a list of domains, separated with the given delimiter, and placing them into
    /// either the domain blacklist or whitelist of the filter depending on the syntax (~ = whitelist, default = blacklist)
    void parseDomains(const QString &domainString, QChar delimiter, AdBlockFilter *filter) const;

    /// Parses a comma separated list of options contained within the given string
    void parseOptions(const QString &optionString, AdBlockFilter *filter) const;

    /// Parses the given AdBlock Plus -formatted regular expression, returning the equivalent string used for a QRegularExpression
    QString parseRegExp(const QString &regExpString) const;
};

#endif // ADBLOCKFILTERPARSER_H
