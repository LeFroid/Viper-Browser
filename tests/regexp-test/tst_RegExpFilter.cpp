#include "timestamp.h"

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

    QRegularExpression getMatchRegExp(const QString &str)
    {
        QChar kleeneStar = QLatin1Char('*');
        int pos = str.indexOf("://");

        // If string is empty or does not specify scheme, return regex accepting anything with http or https scheme
        if (str.isEmpty() || pos <= 0)
            return QRegularExpression(QStringLiteral("http(s?)://"));
        else if (str.compare("<all_urls>") == 0)
            return QRegularExpression(QStringLiteral("(http(s?)|ftp|file)://.*"));

        // Get scheme in the form of a regular expression
        QString converted = str.left(pos);
        if (converted.compare(kleeneStar) == 0)
            converted = QStringLiteral("http(s?)");
        if (!converted.startsWith("http") && !converted.startsWith("ftp") && !converted.startsWith("file"))
            return QRegularExpression(QStringLiteral("http(s?)://"));
        converted.append(QStringLiteral("://"));

        // Get host in the form of a regular expression
        int pathPos = str.indexOf('/', pos + 3);
        if (pathPos < 0)
        {
            // Path character is mandatory
            return QRegularExpression(QStringLiteral("http(s?)://"));
        }
        else if (pathPos > 0)
        {
            QString host = str.left(pathPos);
            host = host.mid(pos + 3);

            int lastStarPos = host.lastIndexOf(kleeneStar);
            if (lastStarPos > 0)
                return QRegularExpression(QStringLiteral("http(s?)://"));

            if (lastStarPos == 0)
            {
                if (host.size() == 1)
                    converted.append(QStringLiteral(".*/"));
                else if (host.at(1) == QLatin1Char('.'))
                {
                    converted.append(".*");
                    host = host.mid(1);
                }
                else
                    return QRegularExpression(QStringLiteral("http(s?)://"));

            }

            converted.append(host);
        }

        converted.append(QChar('/'));

        // Get path in the form of a regular expression
        if (pathPos + 1 == str.size())
            return QRegularExpression(converted);

        QString path = str.mid(pathPos + 1);
        path.replace("*", ".*");
        converted.append(path);
        return QRegularExpression(converted);
    }

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

    /**
     * Tests the getMatchRegExp method with strings that should yield a positive match.
     */
    void testMatchPatternMatches();

    /// Tests the getMatchRegExp method with invalid match patterns and strings that shouldnt match valid patterns
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

void RegExpFilter::testMatchPatternMatches()
{
    QRegularExpression googlExpr = getMatchRegExp(QStringLiteral("https://*.google.com/foo*bar"));
    QString googleExprErrorMsg
            = QString("Regular expression calculated is invalid. Original string: https://*.google.com/foo*bar , Regular Expression: %1").arg(googlExpr.pattern());
    QVERIFY2(googlExpr.isValid(), googleExprErrorMsg.toStdString().c_str());
    qDebug() << "Match pattern: https://*.google.com/foo*bar translated to " << googlExpr.pattern();
    QVERIFY2(googlExpr.match(QStringLiteral("https://docs.google.com/foobar")).hasMatch(), "Pattern should match https://docs.google.com/foobar");
    QVERIFY2(googlExpr.match(QStringLiteral("https://www.google.com/foo/baz/bar")).hasMatch(), "Pattern should match https://www.google.com/foo/baz/bar");

    QRegularExpression googleMailExpr = getMatchRegExp(QStringLiteral("*://mail.google.com/*"));
    QString googleMailExprErrorMsg
            = QString("Regular expression calculated is invalid. Original string: *://mail.google.com/* , Regular Expression: %1").arg(googleMailExpr.pattern());
    QVERIFY2(googleMailExpr.isValid(), googleMailExprErrorMsg.toStdString().c_str());
    qDebug() << "Match pattern: *://mail.google.com/* translated to " << googleMailExpr.pattern();

    QRegularExpression localFileExpr = getMatchRegExp(QStringLiteral("file:///foo*"));
    QString localFileExprErrorMsg =
            QString("Regular expression calculated is invalid. Original string: file:///foo* , Regular Expression: %1").arg(localFileExpr.pattern());
    QVERIFY2(localFileExpr.isValid(), localFileExprErrorMsg.toStdString().c_str());
    QVERIFY2(localFileExpr.match(QStringLiteral("file:///foo/bar.html")).hasMatch(), "Pattern should match file:///foo/bar.html");
    QVERIFY2(localFileExpr.match(QStringLiteral("file:///foo")).hasMatch(), "Pattern should match file:///foo");
    qDebug() << "Match pattern: file:///foo* translated into " << localFileExpr.pattern();
}

void RegExpFilter::testMatchPatternBad()
{
    QRegularExpression googlExprBad = getMatchRegExp(QStringLiteral("http://www.google.com"));
    QVERIFY2(googlExprBad.pattern().compare(QStringLiteral("http(s?)://")) == 0, "Match pattern http://www.google.com should be invalid (missing path character)");

    QRegularExpression googleMailExpr = getMatchRegExp(QStringLiteral("*://mail.google.com/*"));
    QVERIFY2(!googleMailExpr.match(QStringLiteral("ftp://mail.google.com/")).hasMatch(), "Wildcard scheme should not match FTP schemes");
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
