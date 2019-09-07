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

private slots:
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

        QTest::qWait(500);

        // Match by url and then by url tokens
        QString searchTerm("BROWSER.COM");

        FastHashParameters hashParams;
        hashParams.needle = searchTerm.toStdWString();
        hashParams.differenceHash = FastHash::getDifferenceHash(static_cast<quint64>(searchTerm.size()));
        hashParams.needleHash = FastHash::getNeedleHash(hashParams.needle);

        std::vector<URLSuggestion> result =
                suggestor.getSuggestions(working, searchTerm, CommonUtil::tokenizePossibleUrl(searchTerm), hashParams);

        QVERIFY2(result.size() == 1, "Expected result set to have a single entry");

        const URLSuggestion &suggestion = result[0];
        QVERIFY2(suggestion.URL.compare(QLatin1String("https://viper-browser.com")) == 0, "URL Should match expectation");
        QVERIFY2(suggestion.Title.compare(QLatin1String("Viper Browser")) == 0, "Title should match expectation");
        QVERIFY(suggestion.Type == MatchType::URL);
        QVERIFY(suggestion.VisitCount == 1);
        QVERIFY(!suggestion.IsHostMatch);
        QVERIFY(suggestion.URLTypedCount == 1);
    }

    void testThatEntriesMatchByTitle()
    {
        // title + words/tokens in title
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
