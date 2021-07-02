#ifndef _PUBLIC_SUFFIX_RULE_H_
#define _PUBLIC_SUFFIX_RULE_H_

#include <QString>

enum class PublicSuffixRuleType
{
    Normal,
    Wildcard,
    Exception,
    None        // Special case to fit into the tree structure
};

struct PublicSuffixRule
{
    PublicSuffixRuleType type {PublicSuffixRuleType::None};
    int numLabels {0};
    QString value {};
};

#endif // _PUBLIC_SUFFIX_RULE_H_ 

