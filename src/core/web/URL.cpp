#include "public_suffix/PublicSuffixManager.h"
#include "URL.h"

URL::URL() :
    QUrl()
{
}

URL::URL(const QUrl &other) :
    QUrl(other)
{
}

URL::URL(const QString &url, URL::ParsingMode parsingMode) :
    QUrl(url, parsingMode)
{
}

QString URL::getSecondLevelDomain() const
{
    PublicSuffixManager &suffixManager = PublicSuffixManager::instance();
    const QString topLevelDomain = suffixManager.findTld(host().toLower());
    const QString host = this->host();

    if (topLevelDomain.isEmpty() || host.isEmpty())
        return QString();

    QString domain = host.left(host.size() - topLevelDomain.size());

    if (domain.count(QChar('.')) == 0)
        return host;

    while (domain.count(QChar('.')) > 1)
        domain = domain.mid(domain.indexOf(QChar('.')) + 1);

    return domain + topLevelDomain;
}
