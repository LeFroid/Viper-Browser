#include "public_suffix/PublicSuffixTreeNode.h"
#include <iterator>
#include <utility>

PublicSuffixTreeNode::PublicSuffixTreeNode(PublicSuffixRule &&rule, QString &&label) noexcept :
    TreeNode<PublicSuffixTreeNode>(),
    m_label(std::move(label)),
    m_rule(std::move(rule))
{
}

std::vector<PublicSuffixTreeNode*> PublicSuffixTreeNode::findMatching(QStringList domainLabels) const
{
    std::vector<PublicSuffixTreeNode*> result {};

    if (domainLabels.empty())
        return result;

    const QString currentLabel { domainLabels.back() };
    domainLabels.pop_back();

    for (const auto &node : m_children)
    {
        PublicSuffixTreeNode *nodePtr = node.get();
        const QString &childLabel = nodePtr->getLabel();
        if (childLabel.compare(currentLabel) == 0
                || childLabel.compare(QStringLiteral("*")) == 0)
            result.push_back(nodePtr);
    }

    // Recursive search
    const size_t count = result.size();
    for (size_t i = 0; i < count; ++i)
    {
        std::vector<PublicSuffixTreeNode*> nestedMatches = result.at(i)->findMatching(domainLabels);
        if (!nestedMatches.empty())
        {
            result.insert(result.end(),
                          std::make_move_iterator(nestedMatches.begin()),
                          std::make_move_iterator(nestedMatches.end()));
        }
    }

    return result;
}

const QString &PublicSuffixTreeNode::getLabel() const
{
    return m_label;
}

const PublicSuffixRule &PublicSuffixTreeNode::getRule() const
{
    return m_rule;
}
