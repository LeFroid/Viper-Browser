#include "AdBlockFilter.h"
#include "AdBlockFilterParser.h"

#include <memory>
#include <QString>
#include <QtTest>
#include <QUrl>

class AdBlockFilterTest : public QObject
{
    Q_OBJECT

public:
    AdBlockFilterTest();

    QString getSecondLevelDomain(const QUrl &url) const;

private Q_SLOTS:
    //void initTestCase();
    //void cleanupTestCase();
    void testCase1();

private:
    std::unique_ptr<AdBlockFilter> domainCSSFilter;
};

AdBlockFilterTest::AdBlockFilterTest()
{
    AdBlockFilterParser parser;
    domainCSSFilter = parser.makeFilter(QLatin1String("slashdot.org##.ntv-sponsored"));
}

QString AdBlockFilterTest::getSecondLevelDomain(const QUrl &url) const
{
    const QString topLevelDomain = url.topLevelDomain();
    const QString host = url.host();

    if (topLevelDomain.isEmpty() || host.isEmpty())
        return QString();

    QString domain = host.left(host.size() - topLevelDomain.size());

    if (domain.count(QChar('.')) == 0)
        return host;

    while (domain.count(QChar('.')) != 0)
        domain = domain.mid(domain.indexOf(QChar('.')) + 1);

    return domain + topLevelDomain;
}

void AdBlockFilterTest::testCase1()
{
    QUrl shouldMatchUrl1 = QUrl::fromUserInput(QLatin1String("https://developers.slashdot.org/story/18/07/07/0342201/is-c-a-really-terrible-language"));
    QString domain1 = shouldMatchUrl1.host().toLower();

    QUrl shouldMatchUrl2 = QUrl::fromUserInput(QLatin1String("https://slashdot.org/popular"));
    QString domain2 = shouldMatchUrl2.host().toLower();

    QVERIFY2(domainCSSFilter->isDomainStyleMatch(domain1) && !domainCSSFilter->isException(), "The Domain CSS filter did not match the target url");

    QVERIFY2(domainCSSFilter->isDomainStyleMatch(domain2) && !domainCSSFilter->isException(), "The Domain CSS filter did not match the target url");
}

QTEST_APPLESS_MAIN(AdBlockFilterTest)

#include "tst_AdBlockFilterTest.moc"
