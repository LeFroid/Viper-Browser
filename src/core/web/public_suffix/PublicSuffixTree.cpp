#include "public_suffix/PublicSuffixTree.h"
#include "public_suffix/PublicSuffixTreeNode.h"
#include <algorithm>
#include <QStringList>
#include <QDebug>

PublicSuffixTree::PublicSuffixTree() :
    m_root{PublicSuffixRule{}, QString{}}
{
}

PublicSuffixRule PublicSuffixTree::find(const QString &domain) const
{
    const QStringList domainLabels = domain.split(u'.');
    std::vector<PublicSuffixTreeNode*> matches = m_root.findMatching(domainLabels);

    if (matches.empty())
        return {};

    std::vector<PublicSuffixRule> rules;
    rules.reserve(matches.size());

    for (const PublicSuffixTreeNode *node : matches)
    {
        const PublicSuffixRule &rule = node->getRule();
        if (rule.type != PublicSuffixRuleType::None)
            rules.push_back(rule);
    }

    std::sort(rules.begin(), rules.end(), [](const PublicSuffixRule &a, const PublicSuffixRule &b){
        if (a.type == PublicSuffixRuleType::Exception && b.type != PublicSuffixRuleType::Exception)
            return true;
        else if (a.type != PublicSuffixRuleType::Exception && b.type == PublicSuffixRuleType::Exception)
            return false;

        return a.numLabels > b.numLabels;
    });

    return rules.at(0);
}

void PublicSuffixTree::insert(PublicSuffixRule &&rule, QString &&label)
{
    PublicSuffixTreeNode *parent = findParentOf(rule);
    if (parent == nullptr)
    {
        qWarning() << "Unable to find a parent node for the given public suffix rule: " << rule.value
                   << "This should not happen!";
        return;
    }

    parent->appendNode(std::make_unique<PublicSuffixTreeNode>(std::move(rule), std::move(label)));
}

PublicSuffixTreeNode *PublicSuffixTree::findParentOf(const PublicSuffixRule &rule)
{
    PublicSuffixTreeNode *current = &m_root;

    if (rule.numLabels == 1)
        return current;

    QStringList labels = rule.value.split(u'.');
    for (int i = 1; i < rule.numLabels; ++i)
    {
        PublicSuffixTreeNode *next = nullptr;
        QString label { labels.back() };

        for (int j = 0; j < current->getNumChildren(); ++j)
        {
            PublicSuffixTreeNode *child = current->getNode(j);
            if (child->getLabel().compare(label) == 0)
            {
                next = child;
                break;
            }
        }

        // Sanity check
        if (next != nullptr)
            current = next;
        else
        {
            next = current->appendNode(std::make_unique<PublicSuffixTreeNode>(PublicSuffixRule{}, std::move(label)));
            current = next;
        }

        labels.pop_back();
    }

    return current;
}
