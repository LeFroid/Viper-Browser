#include "AdBlockFilter.h"
#include "AdBlockFilterParser.h"

#include <memory>
#include <QString>
#include <QtTest>
#include <QUrl>

#include "URL.h"

using namespace adblock;

class AdBlockFilterTest : public QObject
{
    Q_OBJECT

public:
    AdBlockFilterTest();

private Q_SLOTS:
    void testCosmeticFilterMatch();
    void testFilterOptionMatches();
    void testRedirectFilterMatch();

private:
    std::unique_ptr<Filter> domainCSSFilter;

    std::unique_ptr<Filter> blockDomainRule;
    std::unique_ptr<Filter> allowDomainRule;

    std::unique_ptr<Filter> redirectScriptRule;

    std::unique_ptr<Filter> blockScriptDomainRule;
};

AdBlockFilterTest::AdBlockFilterTest()
{
    FilterParser parser(nullptr);
    domainCSSFilter = parser.makeFilter(QLatin1String("slashdot.org##.ntv-sponsored"));

    blockDomainRule = parser.makeFilter(QLatin1String("|https://$image,media,script,third-party,domain=watchvid.com"));
    allowDomainRule = parser.makeFilter(QLatin1String("@@||mycdn.com^$image,media,object,stylesheet,domain=watchvid.com"));

    redirectScriptRule = parser.makeFilter(QLatin1String("||google-analytics.com/ga.js$script,redirect=google-analytics.com/ga.js"));

    blockScriptDomainRule = parser.makeFilter(QLatin1String("||mssl.fwmrm.net$script,domain=zerohedge.com"));
}

void AdBlockFilterTest::testCosmeticFilterMatch()
{
    QUrl shouldMatchUrl1 = QUrl::fromUserInput(QLatin1String("https://developers.slashdot.org/story/18/07/07/0342201/is-c-a-really-terrible-language"));
    QString domain1 = shouldMatchUrl1.host().toLower();

    QUrl shouldMatchUrl2 = QUrl::fromUserInput(QLatin1String("https://slashdot.org/popular"));
    QString domain2 = shouldMatchUrl2.host().toLower();

    QVERIFY2(domainCSSFilter->isDomainStyleMatch(domain1) && !domainCSSFilter->isException(), "The Domain CSS filter did not match the target url");

    QVERIFY2(domainCSSFilter->isDomainStyleMatch(domain2) && !domainCSSFilter->isException(), "The Domain CSS filter did not match the target url");
}

void AdBlockFilterTest::testFilterOptionMatches()
{
    URL allowedUrl { QUrl::fromUserInput(QLatin1String("https://subdomain.mycdn.com/videos/thumbnails/5.jpg")) };

    QString requestUrl = allowedUrl.toString(QUrl::FullyEncoded).toLower();
    QString firstPartyUrl = QLatin1String("https://www.watchvid.com/watch?id=123456");

    URL wrapperUrl(firstPartyUrl);
    QString baseUrl = wrapperUrl.getSecondLevelDomain().toLower();

    if (baseUrl.isEmpty())
        baseUrl = QUrl(firstPartyUrl).host().toLower();

    ElementType elemType = ElementType::Image | ElementType::ThirdParty;

    // Perform document type and third party type checking
    QString domain = allowedUrl.host().toLower();
    if (domain.startsWith(QLatin1String("www.")))
        domain = domain.mid(4);
    if (domain.isEmpty())
        domain = allowedUrl.getSecondLevelDomain().toLower();

    QVERIFY2(allowDomainRule->isMatch(baseUrl, requestUrl, domain, elemType), "Allow rule should match the request");
    QVERIFY2(blockDomainRule->isMatch(baseUrl, requestUrl, domain, elemType), "Block rule should match the request");
    
    baseUrl = QLatin1String("zerohedge.com");
    requestUrl = QLatin1String("https://mssl.fwmrm.net/p/nbcu_live/AdManager.js");
    domain = QLatin1String("mssl.fwmrm.net");
    elemType = ElementType::Script | ElementType::ThirdParty;
    QVERIFY2(blockScriptDomainRule->isMatch(baseUrl, requestUrl, domain, elemType), "Block rule should match the request"); 
}

void AdBlockFilterTest::testRedirectFilterMatch()
{
    URL requestUrl { QUrl::fromUserInput(QLatin1String("https://ssl.google-analytics.com/ga.js")) };
    QString requestUrlStr = requestUrl.toString(QUrl::FullyEncoded).toLower();

    URL firstPartyUrl { QUrl::fromUserInput(QLatin1String("https://unrelatedsite.com")) };

    QString baseUrl = firstPartyUrl.getSecondLevelDomain().toLower();
    ElementType elemType = ElementType::Script | ElementType::ThirdParty;

    QString domain = requestUrl.host().toLower();
    if (domain.startsWith(QLatin1String("www.")))
        domain = domain.mid(4);
    if (domain.isEmpty())
        domain = requestUrl.getSecondLevelDomain().toLower();

    QVERIFY2(redirectScriptRule->isMatch(baseUrl, requestUrlStr, domain, elemType), "Block rule should match the request");
}

QTEST_APPLESS_MAIN(AdBlockFilterTest)

#include "AdBlockFilterTest.moc"
