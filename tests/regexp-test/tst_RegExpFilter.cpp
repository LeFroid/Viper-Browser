#include "timestamp.h"
#include "CommonUtil.h"

#include <QString>
#include <QStringList>
#include <QtTest>

#include <QRegularExpression>
#include <QRegularExpressionMatch>
#include <QDebug>

class RegExpFilter : public QObject
{
    Q_OBJECT

public:
    RegExpFilter();

private Q_SLOTS:
    /// Tests the validity of the regular expressions being used for testing. Test passes if all reg exps are valid
    void testValidityOfRegExp();

    /**
     * Tests the element hiding filter regexp. If each string in a list, each containing a '##',
     * is found by the regular expression, the test passes.
     */
    void testElementHideShouldMatchRegExp();

    /**
     * Tests the element hiding filter regexp. If each string in a list, none containing a '##',
     * does not match the regular expression, the test passes.
     */
    void testElementHideShouldNotMatchRegExp();

    /// Generates the data used for testing the validity and correctness of various match patterns
    void testMatchPatternMatches_data();

    /**
     * Tests the getRegExpForMatchPattern function with strings that should yield a positive match.
     */
    void testMatchPatternMatches();

    /// Tests the getRegExpForMatchPattern function with invalid match patterns and strings that shouldnt match valid patterns
    void testMatchPatternBad();

private:
    /// CSS element filter rule as a regular expression
    QRegularExpression m_elementRegExp;
};

RegExpFilter::RegExpFilter() :
    m_elementRegExp("^([^/*|@\"!]*?)#([@?])?#(.+)$")
{
}

void RegExpFilter::testValidityOfRegExp()
{
    QVERIFY2(m_elementRegExp.isValid(), "Regular expression for element hiding is invalid");
}

void RegExpFilter::testMatchPatternMatches_data()
{
    QTest::addColumn<QString>("original");
    QTest::addColumn<QRegularExpression>("pattern");
    QTest::addColumn<QString>("target");

    QString googleMatchPattern = QStringLiteral("https://*.google.com/foo*bar");
    QRegularExpression googlExpr = CommonUtil::getRegExpForMatchPattern(googleMatchPattern);
    QTest::newRow("google doc foobar match") << googleMatchPattern << googlExpr << "https://docs.google.com/foobar";
    QTest::newRow("google foo baz bar match") << googleMatchPattern << googlExpr << "https://www.google.com/foo/baz/bar"; 
    
    QString googleMailPattern = QStringLiteral("*://mail.google.com/*");
    QRegularExpression googleMailExpr = CommonUtil::getRegExpForMatchPattern(googleMailPattern);
    QTest::newRow("google mail match") << googleMailPattern << googleMailExpr << "https://mail.google.com/mail/u/0/";
    QTest::newRow("google mail message match") << googleMailPattern << googleMailExpr << "https://mail.google.com/mail/u/0/#someMessage";
    
    QString localFilePattern = QStringLiteral("file:///foo*");
    QRegularExpression localFileExpr = CommonUtil::getRegExpForMatchPattern(localFilePattern);
    QTest::newRow("file match wildcard ending") << localFilePattern << localFileExpr << "file:///foo/bar.html";
    QTest::newRow("file match exact") << localFilePattern << localFileExpr << "file:///foo";
}

void RegExpFilter::testMatchPatternMatches()
{
    QFETCH(QString, original);
    QFETCH(QRegularExpression, pattern);
    QFETCH(QString, target);

    QString validityQString = QString("Regular expression calculated from match pattern is invalid. Original string: %1, Derived Pattern: %2").arg(original).arg(pattern.pattern());
    const char *validityErrMsg = validityQString.toStdString().c_str();
    QVERIFY2(pattern.isValid(), validityErrMsg);

    QString hasMatchQString = QString("Pattern %1 should match %2").arg(original).arg(target);
    const char *hasMatchErrMsg = hasMatchQString.toStdString().c_str();
    QVERIFY2(pattern.match(target).hasMatch(), hasMatchErrMsg);
}

void RegExpFilter::testMatchPatternBad()
{
    QRegularExpression googlExprBad = CommonUtil::getRegExpForMatchPattern(QStringLiteral("http://www.google.com"));
    QVERIFY2(googlExprBad.pattern().compare(QStringLiteral("http(s?)://")) == 0, "Match pattern http://www.google.com should be invalid (missing path character)");

    QRegularExpression googleMailExpr = CommonUtil::getRegExpForMatchPattern(QStringLiteral("*://mail.google.com/*"));
    QVERIFY2(!googleMailExpr.match(QStringLiteral("ftp://mail.google.com/")).hasMatch(), "Wildcard scheme should not match FTP schemes");

    QRegularExpression localFileExpr = CommonUtil::getRegExpForMatchPattern(QStringLiteral("file:///foo*"));
    QVERIFY2(!localFileExpr.match(QStringLiteral("file:///home/user/foo/list.txt")).hasMatch(), "Match pattern file:///foo* should only match file URLs when the path starts with the string foo");
    QVERIFY2(!localFileExpr.match(QStringLiteral("https://foo.website.com/test/")).hasMatch(), "Match pattern file:///foo* should only match file schemes");
}

void RegExpFilter::testElementHideShouldMatchRegExp()
{
    // Create a list of strings that should match the css element hiding regular expression
    QStringList elemMatchList;
    elemMatchList << "example.com,~foo.example.com##.sponsor"
                  << "domain3.example##.sponsor"
                  << "###A9AdsServicesWidgetTop"
                  << "coed.com##.cmg-nesn-embed"
                  << "##[onclick*=\"content.ad/\"]";
    int expectedMatches = elemMatchList.size();
    QStringList resultList;
    QRegularExpressionMatch match;
    //Timestamps to see how much time a regular expression match takes
    timestamp_t t0, t1;

    for (const QString &str : elemMatchList)
    {
        t0 = get_timestamp();
        match = m_elementRegExp.match(str, 0);
        t1 = get_timestamp();
        QString errMsg = QString("Element hiding RegExp did not match string. String: %1").arg(str);
        QVERIFY2(match.hasMatch(), errMsg.toStdString().c_str());
        qDebug() << "RegExp.match(" << str << ") took " << t1 - t0 << " microseconds";
        resultList << match.captured(3);
    }
    qDebug() << "Resulting strings after applying regexp: ";
    for (const QString &str : resultList)
        qDebug() << str;
    QVERIFY2(resultList.size() == expectedMatches, "Size of result list not equal to expected size. RegExp Failure");
}

void RegExpFilter::testElementHideShouldNotMatchRegExp()
{
    // Create a list of strings that should match the css element hiding regular expression
    QStringList elemNoMatchList;
    elemNoMatchList << "example.com,~foo.example.com"
                  << "domain3.example/page?param1=x&param2=y"
                  << "/A9AdsServicesWidgetTop"
                  << "https://coed.com"
                  << "@@||google.com^";

    QRegularExpressionMatch match;

    //Timestamps to see how much time a regular expression match takes
    timestamp_t t0, t1;

    for (const QString &str : elemNoMatchList)
    {
        t0 = get_timestamp();
        match = m_elementRegExp.match(str, 0);
        t1 = get_timestamp();
        QString errMsg = QString("Element hiding RegExp matched string without the element rule. String: %1").arg(str);
        QVERIFY2(!match.hasMatch(), errMsg.toStdString().c_str());
        qDebug() << "RegExp.match(" << str << ") call took " << t1 - t0 << " microseconds";
    }
}

QTEST_APPLESS_MAIN(RegExpFilter)

#include "tst_RegExpFilter.moc"
