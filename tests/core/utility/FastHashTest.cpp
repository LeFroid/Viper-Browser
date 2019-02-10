#include "FastHash.h"
#include "CommonUtil.h"

#include <algorithm>
#include <random>
#include <functional>
#include <vector>

#include <QString>
#include <QtTest>
#include <QDebug>

// Random string generation based on https://stackoverflow.com/questions/440133/how-do-i-create-a-random-alpha-numeric-string-in-c
using char_array = std::vector<char>;
char_array charset()
{
    return char_array( 
    {'0','1','2','3','4',
    '5','6','7','8','9',
    'A','B','C','D','E','F',
    'G','H','I','J','K',
    'L','M','N','O','P',
    'Q','R','S','T','U',
    'V','W','X','Y','Z',
    'a','b','c','d','e','f',
    'g','h','i','j','k',
    'l','m','n','o','p',
    'q','r','s','t','u',
    'v','w','x','y','z'
    });
};

std::string random_string(size_t length, std::function<char(void)> rand_char)
{
    std::string str(length, 0);
    std::generate_n(str.begin(), length, rand_char);
    return str;
}

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
    QTest::newRow("quick brown fox") << "fox jumped" << "The quick brown fox jumped over the lazy dog";

    // Make 10 random strings - only generating 10 as the fast hash is benchmarked, and each row of data
    // is ran tens to hundreds of thousands of times to test correctness and performance
    const int numRandomStrings = 10;
    const auto charSet = charset();
    std::default_random_engine rng(std::random_device{}());
    std::uniform_int_distribution<> dist(0, charSet.size() - 1);
    auto randchar = [charSet, &dist,&rng](){
        return charSet[dist(rng)];
    };

    std::uniform_int_distribution<size_t> randStringLen(80, 200);
    std::uniform_int_distribution<size_t> randNeedleLen(5, 70);
    for (int i = 0; i < numRandomStrings; ++i)
    {
        size_t stringLen = randStringLen(rng);
        size_t needleLen = randNeedleLen(rng);

        auto randomString = random_string(stringLen, randchar);

        std::uniform_int_distribution<size_t> randNeedleOffset(0, randomString.size() - needleLen);
        size_t needleOffset = randNeedleOffset(rng);
        auto randomNeedle = randomString.substr(needleOffset, needleLen);

        QString testName = QString("Random string %1").arg(i);
        QTest::newRow(testName.toStdString().c_str()) << randomNeedle.c_str() << randomString.c_str();
    }
}

void FastHashTest::testStringsShouldMatch()
{
    QFETCH(QString, needle);
    QFETCH(QString, haystack);

    auto needleWideStr = needle.toStdWString();
	auto haystackWideStr = haystack.toStdWString();
    quint64 differenceHash = FastHash::getDifferenceHash(static_cast<quint64>(needle.size()));
	quint64 needleHash = FastHash::getNeedleHash(needleWideStr);

    bool isMatch = false;
    QBENCHMARK {
	    isMatch = FastHash::isMatch(needleWideStr, haystackWideStr, needleHash, differenceHash);
    }
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
    QTest::newRow("case sensitive fox") << "fox jumPed" << "The quick brown fox jumped over the lazy dog";
}

void FastHashTest::testStringsShouldNotMatch()
{
    QFETCH(QString, needle);
    QFETCH(QString, haystack);

    auto needleWideStr = needle.toStdWString();
	auto haystackWideStr = haystack.toStdWString();
    quint64 differenceHash = FastHash::getDifferenceHash(static_cast<quint64>(needle.size()));
	quint64 needleHash = FastHash::getNeedleHash(needleWideStr);

    bool isMatch = true;
    QBENCHMARK {
	    isMatch = FastHash::isMatch(needleWideStr, haystackWideStr, needleHash, differenceHash);
    }

	QString errMsgOnFail = QString("False positive: the Rabin-Karp string matcher should not have found the needle %1 in the haystack %2").arg(needle).arg(haystack);
	const char *errMsgCStr = errMsgOnFail.toStdString().c_str();
	QVERIFY2(!isMatch, errMsgCStr);
}

QTEST_APPLESS_MAIN(FastHashTest)

#include "FastHashTest.moc"

