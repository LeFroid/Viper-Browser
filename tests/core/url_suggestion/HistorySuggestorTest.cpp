#include "CommonUtil.h"
#include "DatabaseFactory.h"
#include "FastHash.h"
#include "FaviconManager.h"
#include "HistoryManager.h"
#include "HistoryStore.h"
#include "HistorySuggestor.h"
#include "ServiceLocator.h"
#include "URLSuggestion.h"

#include <atomic>
#include <QDateTime>
#include <QFile>
#include <QObject>
#include <QSqlDatabase>
#include <QTest>

const static QString TEST_FAVICON_DB_FILE = QStringLiteral("HISTORY_SUGGESTOR_TEST_FAVICON.db");
const static QString TEST_DB_FILE = QStringLiteral("HISTORY_SUGGESTOR_TEST.db");

class HistorySuggestorTest : public QObject
{
    Q_OBJECT

public:
    HistorySuggestorTest() :
        QObject(nullptr)
    {
    }

private:
    FastHashParameters getHashParams(const QString &str)
    {
        FastHashParameters result;
        result.needle = str.toStdWString();
        result.differenceHash = FastHash::getDifferenceHash(static_cast<quint64>(str.size()));
        result.needleHash = FastHash::getNeedleHash(result.needle);
        return result;
    }

private Q_SLOTS:
    /// Called before any tests are executed
    void initTestCase()
    {
        if (QFile::exists(TEST_DB_FILE))
            QFile::remove(TEST_DB_FILE);

        if (QFile::exists(TEST_FAVICON_DB_FILE))
            QFile::remove(TEST_FAVICON_DB_FILE);
    }

    /// Called after every test function. This closes the DB connection and deallocates / clears
    /// the HistoryStore instance
    void cleanup()
    {
        if (QFile::exists(TEST_DB_FILE))
            QFile::remove(TEST_DB_FILE);

        if (QFile::exists(TEST_FAVICON_DB_FILE))
            QFile::remove(TEST_FAVICON_DB_FILE);

        QSqlDatabase::removeDatabase(QLatin1String("Favicons"));
        QSqlDatabase::removeDatabase(QLatin1String("HistoryDB"));

        QTest::qWait(250);
    }

    void testThatEntriesMatchByUrl()
    {
        DatabaseTaskScheduler taskScheduler;
        taskScheduler.addWorker("HistoryStore", std::bind(DatabaseFactory::createDBWorker<HistoryStore>, TEST_DB_FILE));

        ViperServiceLocator serviceLocator;

        FaviconManager faviconManager(TEST_FAVICON_DB_FILE);
        HistoryManager historyManager(serviceLocator, taskScheduler);

        QVERIFY(serviceLocator.addService(faviconManager.objectName().toStdString(), &faviconManager));
        QVERIFY(serviceLocator.addService(historyManager.objectName().toStdString(), &historyManager));

        taskScheduler.run();

        // Let initialization routine complete
        QTest::qWait(500);

        // Add a couple of entries to the database
        QUrl firstUrl { QUrl::fromUserInput("https://viper-browser.com") },
             secondUrl { QUrl::fromUserInput("https://a.datacenter.website.net/landing") };

        historyManager.addVisit(firstUrl, QLatin1String("Viper Browser"), QDateTime::currentDateTime(), firstUrl, true);
        historyManager.addVisit(secondUrl, QLatin1String("Other Webpage"), QDateTime::currentDateTime(), secondUrl, true);

        // Finally instantiate the history suggestor
        HistorySuggestor suggestor;
        suggestor.setServiceLocator(serviceLocator);
        suggestor.timerEvent();

        std::atomic_bool working { true };

        QTest::qWait(2000);

        // Match by url and then by url tokens
        QString searchTerm("BROWSER.COM");
        FastHashParameters hashParams = getHashParams(searchTerm);

        std::vector<URLSuggestion> result =
                suggestor.getSuggestions(working, searchTerm, CommonUtil::tokenizePossibleUrl(searchTerm), hashParams);

        QVERIFY2(result.size() == 1, "Expected result set to have a single entry");

        {
            const URLSuggestion &suggestion = result[0];
            QVERIFY2(suggestion.URL.compare(QLatin1String("https://viper-browser.com")) == 0, "URL Should match expectation");
            QVERIFY2(suggestion.Title.compare(QLatin1String("Viper Browser")) == 0, "Title should match expectation");
            QVERIFY(suggestion.VisitCount == 1);
            QVERIFY(!suggestion.IsHostMatch);
            QVERIFY(suggestion.URLTypedCount == 1);
        }

        searchTerm = QLatin1String("WEBSITE.NET");
        hashParams = getHashParams(searchTerm);
        result = suggestor.getSuggestions(working, searchTerm, CommonUtil::tokenizePossibleUrl(searchTerm), hashParams);

        QVERIFY2(result.size() == 1, "Expected result set to have a single entry");

        {
            const URLSuggestion &suggestion = result[0];
            QVERIFY2(suggestion.URL.compare(QLatin1String("https://a.datacenter.website.net/landing")) == 0, "URL Should match expectation");
            QVERIFY2(suggestion.Title.compare(QLatin1String("Other Webpage")) == 0, "Title should match expectation");
            QVERIFY(suggestion.VisitCount == 1);
            QVERIFY(!suggestion.IsHostMatch);
            QVERIFY(suggestion.URLTypedCount == 1);
        }

        searchTerm = QLatin1String("DOESNT MATCH");
        hashParams = getHashParams(searchTerm);
        result = suggestor.getSuggestions(working, searchTerm, CommonUtil::tokenizePossibleUrl(searchTerm), hashParams);

        QVERIFY2(result.empty(), "Expected result set to be empty");
    }

    void testThatEntriesMatchByTitle()
    {
        DatabaseTaskScheduler taskScheduler;
        taskScheduler.addWorker("HistoryStore", std::bind(DatabaseFactory::createDBWorker<HistoryStore>, TEST_DB_FILE));

        ViperServiceLocator serviceLocator;

        FaviconManager faviconManager(TEST_FAVICON_DB_FILE);
        HistoryManager historyManager(serviceLocator, taskScheduler);

        QVERIFY(serviceLocator.addService(faviconManager.objectName().toStdString(), &faviconManager));
        QVERIFY(serviceLocator.addService(historyManager.objectName().toStdString(), &historyManager));

        taskScheduler.run();

        // Let initialization routine complete
        QTest::qWait(500);

        // Add a couple of entries to the database
        QUrl firstUrl { QUrl::fromUserInput("https://randomblog.com") },
             secondUrl { QUrl::fromUserInput("https://charity.org/faq") };

        historyManager.addVisit(firstUrl, QLatin1String("Reliable News"), QDateTime::currentDateTime(), firstUrl, true);
        historyManager.addVisit(secondUrl, QLatin1String("Donate Today | FAQ"), QDateTime::currentDateTime().addDays(-1), secondUrl, false);

        QTest::qWait(500);

        // Finally instantiate the history suggestor
        HistorySuggestor suggestor;
        suggestor.setServiceLocator(serviceLocator);
        suggestor.timerEvent();

        std::atomic_bool working { true };

        QTest::qWait(3000);

        // Match by title only
        QString searchTerm("NEWS");
        FastHashParameters hashParams = getHashParams(searchTerm);

        std::vector<URLSuggestion> result =
                suggestor.getSuggestions(working, searchTerm, CommonUtil::tokenizePossibleUrl(searchTerm), hashParams);

        QVERIFY2(result.size() == 1, "Expected result set to have a single entry");

        {
            const URLSuggestion &suggestion = result[0];
            QVERIFY2(suggestion.URL.compare(QLatin1String("https://randomblog.com")) == 0, "URL Should match expectation");
            QVERIFY2(suggestion.Title.compare(QLatin1String("Reliable News")) == 0, "Title should match expectation");
            QVERIFY(suggestion.Type == MatchType::Title);
            QVERIFY(suggestion.VisitCount == 1);
            QVERIFY(!suggestion.IsHostMatch);
            QVERIFY(suggestion.URLTypedCount == 1);
        }

        searchTerm = QLatin1String("DONA FAQ");
        hashParams = getHashParams(searchTerm);

        result = suggestor.getSuggestions(working, searchTerm, CommonUtil::tokenizePossibleUrl(searchTerm), hashParams);

        QVERIFY2(result.size() == 1, "Expected result set to have a single entry");
        {
            const URLSuggestion &suggestion = result[0];
            QVERIFY2(suggestion.URL.compare(QLatin1String("https://charity.org/faq")) == 0, "URL Should match expectation");
            QVERIFY2(suggestion.Title.compare(QLatin1String("Donate Today | FAQ")) == 0, "Title should match expectation");
            QVERIFY(suggestion.Type == MatchType::SearchWords);
            QVERIFY(suggestion.VisitCount == 1);
            QVERIFY(!suggestion.IsHostMatch);
            QVERIFY(suggestion.URLTypedCount == 0);
        }
    }

    void testThatStaleEntriesDontMatch()
    {
        /*
         * if (record.getUrlTypedCount() < 1
                    && record.getNumVisits() < 4
                    && record.getLastVisit() < cutoffTime)
                continue;
        */
    }

    void testThatIrrelevantEntriesDontMatch()
    {

    }
};

QTEST_GUILESS_MAIN(HistorySuggestorTest)

#include "HistorySuggestorTest.moc"
