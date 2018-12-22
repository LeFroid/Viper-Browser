#include "FastHash.h"
#include "CommonUtil.h"

#include <QString>
#include <QtTest>
#include <QDebug>

class FastHashTest : public QObject
{
    Q_OBJECT

public:
    FastHashTest();

private Q_SLOTS:
    void testStringsShouldMatch_data();

    void testStringsShouldMatch();

    void testStringsShouldNotMatch_data();

    void testStringsShouldNotMatch();
};

FastHashTest::FastHashTest()
{
}

void FastHashTest::testStringsShouldMatch_data()
{
    QTest::addColumn<QString>("needle");
    QTest::addColumn<QString>("haystack");

	QTest::newRow("tag manager") << "tagmanager.com/tag.js" << "target.ad.tagmanager.com/tag.js";
    QTest::newRow("cdn") << "somecdn.com/img" << "https://subdomain.somecdn.com/img/a/123/4/xyz.jpg";
}

void FastHashTest::testStringsShouldMatch()
{
    QFETCH(QString, needle);
    QFETCH(QString, haystack);

    auto needleWideStr = needle.toStdWString();
	auto haystackWideStr = haystack.toStdWString();
    quint64 differenceHash = FastHash::getDifferenceHash(static_cast<quint64>(needle.size()));
	quint64 needleHash = FastHash::getNeedleHash(needleWideStr);

	bool isMatch = FastHash::isMatch(needleWideStr, haystackWideStr, needleHash, differenceHash);
	
	QString errMsgOnFail = QString("The Rabin-Karp string matcher failed to find the needle %1 in the haystack %2").arg(needle).arg(haystack);
	const char *errMsgCStr = errMsgOnFail.toStdString().c_str();
	QVERIFY2(isMatch, errMsgCStr);
}

void FastHashTest::testStringsShouldNotMatch_data()
{
    QTest::addColumn<QString>("needle");
    QTest::addColumn<QString>("haystack");

	QTest::newRow("cdn vs cnd") << "somecdn.com/img" << "https://subdomain.somecnd.com/img/a/123/4/xyz.jpg";
	QTest::newRow("hostname") << ".example.com/ads/123.js" << "www.badexample.com/ads/123.js";
}

void FastHashTest::testStringsShouldNotMatch()
{
    QFETCH(QString, needle);
    QFETCH(QString, haystack);

    auto needleWideStr = needle.toStdWString();
	auto haystackWideStr = haystack.toStdWString();
    quint64 differenceHash = FastHash::getDifferenceHash(static_cast<quint64>(needle.size()));
	quint64 needleHash = FastHash::getNeedleHash(needleWideStr);

	bool isMatch = FastHash::isMatch(needleWideStr, haystackWideStr, needleHash, differenceHash);

	QString errMsgOnFail = QString("False positive: the Rabin-Karp string matcher should not have found the needle %1 in the haystack %2").arg(needle).arg(haystack);
	const char *errMsgCStr = errMsgOnFail.toStdString().c_str();
	QVERIFY2(!isMatch, errMsgCStr);
}

QTEST_APPLESS_MAIN(FastHashTest)

#include "tst_FastHash.moc"

