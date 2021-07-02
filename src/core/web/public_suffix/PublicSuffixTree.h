#ifndef _PUBLIC_SUFFIX_TREE_H_
#define _PUBLIC_SUFFIX_TREE_H_

#include "public_suffix/PublicSuffixRule.h"
#include "public_suffix/PublicSuffixTreeNode.h"

class PublicSuffixTree
{
public:
    /// Constructs the empty tree
    PublicSuffixTree();

    /**
     * @brief find Searches the tree for a public suffix rule that applies to the given domain.
     * @param domain Punycode-formatted domain string
     * @return Public suffix rule, if found, otherwise a "null" or no-op rule is returned
     */
    PublicSuffixRule find(const QString &domain) const;

    /**
     * @brief insert Inserts a new tree node with the given parameters
     * @param rule Rule associated with the tree node
     * @param label Label (domain segment) to be associated with the node
     */
    void insert(PublicSuffixRule &&rule, QString &&label);

private:
    /// Returns the parent node for a given public suffix rule.
    /// Ex: If rule.value = "co.uk", parent node = TreeNode{ label="uk", parent=root }
    ///     If rule.value = "*.co.uk", parent node = TreeNode{ label="co", parent=TN{label="uk"} }
    PublicSuffixTreeNode *findParentOf(const PublicSuffixRule &rule);

private:
    /// The root node of the tree
    PublicSuffixTreeNode m_root;
};

#endif // _PUBLIC_SUFFIX_TREE_H_

