#include "public_suffix/PublicSuffixRuleParser.h"

#include <QFile>
#include <QTextStream>

std::vector<PublicSuffixRule> PublicSuffixRuleParser::loadRules(const QString &fileName)
{
    std::vector<PublicSuffixRule> result;

    QFile ruleFile(fileName);
    if (!ruleFile.exists() || !ruleFile.open(QIODevice::ReadOnly))
        return result;

    QString line;
    QTextStream stream(&ruleFile);
    while (stream.readLineInto(&line))
    {
        line = line.trimmed();

        if (line.isEmpty() || line.startsWith(QStringLiteral("//")))
            continue;

        result.emplace_back(parseLine(line));
    }

    // Parsing into tree:
    // 1) Sort this vector in ascending order of rule.numLabels
    // 2) Create empty root tree node
    // 3) Iterate through this vector
    // 4a) For each rule with numLabels == 1, create a child of the empty root node
    // 4b) For each rule with numLabels > 1, find the node with same parent label and append as a child
    return result;
}

PublicSuffixRule PublicSuffixRuleParser::parseLine(QString line)
{
    PublicSuffixRuleType ruleType { PublicSuffixRuleType::Normal };

    const QChar start = line.at(0);
    if (start == u'!')
    {
        line = line.mid(1);
        ruleType = PublicSuffixRuleType::Exception;
    }
    else if (start == u'*')
        ruleType = PublicSuffixRuleType::Wildcard;

    const int numLabels = line.count(u'.') + 1;

    return { ruleType, numLabels, line };
}
