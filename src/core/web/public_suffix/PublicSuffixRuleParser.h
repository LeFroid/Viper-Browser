#ifndef _PUBLIC_SUFFIX_RULE_PARSER_H_
#define _PUBLIC_SUFFIX_RULE_PARSER_H_

#include "public_suffix/PublicSuffixRule.h"

#include <vector>
#include <QString>

/**
 * @class PublicSuffixRuleParser
 * @brief Loads the public suffix list (see: https://publicsuffix.org/) into a set of
 *        rules that can be interpreted by the application
 */
class PublicSuffixRuleParser
{
public:
    /**
     * @brief loadRules Opens the file with the given filename and parses the public suffix rules
     * @param fileName Name of the file (QRC file)
     * @return Vector of rules as defined in the file.
     */
    std::vector<PublicSuffixRule> loadRules(const QString &fileName);

private:
    /// Parses a single line of text for a public suffix rule
    PublicSuffixRule parseLine(QString line);
};

#endif // _PUBLIC_SUFFIX_RULE_PARSER_H_
