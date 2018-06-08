#include "URL.h"

URL::URL() :
    QUrl()
{
}

URL::URL(const QUrl &other) :
    QUrl(other)
{
}

URL::URL(const URL &other) :
    QUrl(other)
{
}

URL::URL(const QString &url, URL::ParsingMode parsingMode) :
    QUrl(url, parsingMode)
{
}

QString URL::getSecondLevelDomain() const
{
    const QString topLevelDomain = this->topLevelDomain();
    const QString host = this->host();

    if (topLevelDomain.isEmpty() || host.isEmpty())
        return QString();

    QString domain = host.left(host.size() - topLevelDomain.size());

    if (domain.count(QChar('.')) == 0)
        return host;

    while (domain.count(QChar('.')) != 0)
        domain = domain.mid(domain.indexOf(QChar('.')) + 1);

    return domain + topLevelDomain;
}
