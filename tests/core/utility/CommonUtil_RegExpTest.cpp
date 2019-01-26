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
    /// Generates the data used for testing the validity and correctness of various match patterns
    void testMatchPatternMatches_data();

    /**
     * Tests the getRegExpForMatchPattern function with strings that should yield a positive match.
     */
    void testMatchPatternMatches();

    /// Tests the getRegExpForMatchPattern function with invalid match patterns and strings that shouldnt match valid patterns
    void testMatchPatternBad();
};

RegExpFilter::RegExpFilter()
{
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

QTEST_APPLESS_MAIN(RegExpFilter)

#include "CommonUtil_RegExpTest.moc"
