#include "public_suffix/PublicSuffixManager.h"
#include "public_suffix/PublicSuffixRuleParser.h"

#include <algorithm>

PublicSuffixManager::PublicSuffixManager(QObject *parent) :
    QObject(parent),
    m_tree()
{
    setObjectName(QStringLiteral("PublicSuffixManager"));
    load();
}

PublicSuffixManager::~PublicSuffixManager()
{
}

PublicSuffixManager &PublicSuffixManager::instance()
{
    static PublicSuffixManager manager;
    return manager;
}

QString PublicSuffixManager::findTld(const QString &domain) const
{
    const PublicSuffixRule rule = m_tree.find(domain);

    QString tld = rule.value;
    if (rule.type == PublicSuffixRuleType::Wildcard)
    {
        tld = tld.mid(1);

        int domainIdx = domain.lastIndexOf(tld);
        if (domainIdx <= 0)
            return QString();

        QString temp = domain.left(domainIdx);
        domainIdx = temp.lastIndexOf(u'.');
        if (domainIdx > 0)
            temp = temp.mid(domainIdx + 1);

        tld.prepend(temp);
    }
    else if (rule.type == PublicSuffixRuleType::Exception)
    {
        tld = tld.mid(tld.indexOf(u'.') + 1);
    }

    return tld;
}

PublicSuffixRule PublicSuffixManager::findRule(const QString &domain) const
{
    return m_tree.find(domain);
}

void PublicSuffixManager::load()
{
    PublicSuffixRuleParser parser;
    std::vector<PublicSuffixRule> rules = parser.loadRules(QStringLiteral(":/public_suffix_list.dat"));

    std::sort(rules.begin(), rules.end(), [](const PublicSuffixRule &a, const PublicSuffixRule &b){
        return a.numLabels < b.numLabels;
    });

    for (PublicSuffixRule &rule : rules)
    {
        QString label = rule.value;

        int delimIdx = label.indexOf(u'.');
        if (delimIdx > 0)
            label = label.left(delimIdx);

        m_tree.insert(std::move(rule), std::move(label));
    }
}
