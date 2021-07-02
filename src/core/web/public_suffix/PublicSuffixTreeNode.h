#ifndef _PUBLIC_SUFFIX_TREE_NODE_H_
#define _PUBLIC_SUFFIX_TREE_NODE_H_

#include "public_suffix/PublicSuffixRule.h"
#include "TreeNode.h"
#include <QString>
#include <QStringList>

class PublicSuffixTreeNode final : public TreeNode<PublicSuffixTreeNode>
{
public:
    /**
     * @brief PublicSuffixTreeNode Constructs the tree node with a corresponding rule and domain segment (label)
     * @param rule Rule that the tree node represents
     * @param label Label of the node at its relative depth in the tree
     */
    explicit PublicSuffixTreeNode(PublicSuffixRule &&rule, QString &&label) noexcept;
    PublicSuffixTreeNode(const PublicSuffixTreeNode &other) = default;
    PublicSuffixTreeNode(PublicSuffixTreeNode &&other) noexcept = default;

    /// Returns a vector of all child nodes that match the domain segment input list
    std::vector<PublicSuffixTreeNode*> findMatching(QStringList domainLabels) const;

    /// Returns the label associated with the tree node
    const QString &getLabel() const;

    /// Returns the rule associated with the tree node
    const PublicSuffixRule &getRule() const;

private:
    /// Label associated with this level of the tree node
    /// Ex: TLD = "co.uk", Level 0 label ="uk", level 1 label = "co"
    QString m_label;

    /// Rule associated with the tree node
    PublicSuffixRule m_rule;
};

#endif // _PUBLIC_SUFFIX_TREE_NODE_H_
