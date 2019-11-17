#include "DatabaseFactory.h"
#include "HistoryStore.h"

#include <QFile>
#include <QObject>
#include <QString>
#include <QTest>

class HistoryStoreTest : public QObject
{
    Q_OBJECT

public:
    HistoryStoreTest() :
        QObject(nullptr),
        m_dbFile(QLatin1String("HistoryStoreTest.db"))
    {
    }

private slots:
    /// Called before any tests are executed
    void initTestCase()
    {
        if (QFile::exists(m_dbFile))
            QFile::remove(m_dbFile);
    }

    /// Called after every test function. This closes the DB connection and deallocates / clears
    /// the HistoryStore instance
    void cleanup()
    {
        if (QFile::exists(m_dbFile))
            QFile::remove(m_dbFile);
    }

    /// Tests that entries can be added to the history store
    void testAddVisits()
    {
        std::unique_ptr<HistoryStore> historyStore = DatabaseFactory::createWorker<HistoryStore>(m_dbFile);

        QUrl firstUrl { QUrl::fromUserInput("https://viper-browser.com") }, firstUrlRequested { QUrl::fromUserInput("viper-browser.com") };
        QDateTime firstDate  = QDateTime::currentDateTime();
        historyStore->addVisit(firstUrl, QLatin1String("Viper Browser"), firstDate, firstUrlRequested, false);

        QUrl secondUrl { QUrl::fromUserInput("https://a.datacenter.website.net/landing") }, secondUrlRequested { QUrl::fromUserInput("website.net") };
        historyStore->addVisit(secondUrl, QLatin1String("Some Website"), QDateTime::currentDateTime(), secondUrlRequested, false);

        QVERIFY(historyStore->contains(firstUrl));
        //Don't test first url requested, QUrl::fromUserInput translates to http://viper-browser.com, while history store
        //ignores requested URLs with same scheme as actual URL and no other differences
        QVERIFY(historyStore->contains(secondUrl));
        QVERIFY(historyStore->contains(secondUrlRequested));

        HistoryEntry entry = historyStore->getEntry(firstUrl);
        QCOMPARE(entry.URL, firstUrl);
        QCOMPARE(entry.Title, QLatin1String("Viper Browser"));
        QCOMPARE(entry.NumVisits, 1);
        QCOMPARE(entry.LastVisit, firstDate);
    }

    /// Tests that the browsing history can be cleared
    void testClearingHistory()
    {
        std::unique_ptr<HistoryStore> historyStore = DatabaseFactory::createWorker<HistoryStore>(m_dbFile);

        QUrl firstUrl { QUrl::fromUserInput("https://a.datacenter.website.net/landing") }, firstUrlRequested { QUrl::fromUserInput("website.net") };
        historyStore->addVisit(firstUrl, QLatin1String("Some Website"), QDateTime::currentDateTime(), firstUrlRequested, false);

        QVERIFY(historyStore->contains(firstUrl));
        QVERIFY(historyStore->contains(firstUrlRequested));

        historyStore->clearAllHistory();

        QVERIFY2(!historyStore->contains(firstUrl), "HistoryStore::clearAllHistory did not remove the entry");
        QVERIFY2(!historyStore->contains(firstUrlRequested), "HistoryStore::clearAllHistory did not remove the entry");

        QDateTime firstDate  = QDateTime::currentDateTime().addDays(-5);
        QDateTime secondDate = QDateTime::currentDateTime();
        QUrl secondUrl { QUrl::fromUserInput("https://viper-browser.com") }, secondUrlRequested { QUrl::fromUserInput("viper-browser.com") };

        historyStore->addVisit(firstUrl, QLatin1String("Some Website"), firstDate, firstUrlRequested, true);
        historyStore->addVisit(secondUrl, QLatin1String("Viper Browser"), secondDate, secondUrlRequested, true);

        QVERIFY(historyStore->contains(firstUrl));
        QVERIFY(historyStore->contains(secondUrl));

        historyStore->clearHistoryInRange({firstDate, QDateTime::currentDateTime().addDays(-1)});

        QVERIFY2(!historyStore->contains(firstUrl), "HistoryStore::clearHistoryInRange did not remove the entry");
        QVERIFY2(historyStore->contains(secondUrl), "HistoryStore::clearHistoryInRange removed an entry outside of the given range");

        historyStore->clearHistoryFrom(QDateTime::currentDateTime().addDays(-1));
        QVERIFY2(!historyStore->contains(secondUrl), "HistoryStore::clearHistoryFrom did not remove the entry");
    }

    /// Tests that the reported visit count matches the expected values
    void testVisitCount()
    {
        std::unique_ptr<HistoryStore> historyStore = DatabaseFactory::createWorker<HistoryStore>(m_dbFile);

        QUrl firstUrl { QUrl::fromUserInput("https://viper-browser.com") }, firstUrlRequested { QUrl::fromUserInput("viper-browser.com") };
        QUrl secondUrl { QUrl::fromUserInput("https://a.datacenter.website.net/landing") }, secondUrlRequested { QUrl::fromUserInput("website.net") };
        historyStore->addVisit(firstUrl, QLatin1String("Viper Browser"), QDateTime::currentDateTime(), firstUrlRequested, true);
        historyStore->addVisit(secondUrl, QLatin1String("Some Website"), QDateTime::currentDateTime(), secondUrlRequested, true);

        QCOMPARE(historyStore->getTimesVisited(firstUrl), 1);
        QCOMPARE(historyStore->getTimesVisited(secondUrl), 1);

        QCOMPARE(historyStore->getTimesVisitedHost(firstUrl), 1);
        QCOMPARE(historyStore->getTimesVisitedHost(secondUrl), 1);
        QCOMPARE(historyStore->getTimesVisitedHost(secondUrlRequested), 2);
    }

    /// Tests the call to getHistoryFrom(const QDateTime &startDate)
    void testGetHistoryFrom()
    {
        std::unique_ptr<HistoryStore> historyStore = DatabaseFactory::createWorker<HistoryStore>(m_dbFile);

        QUrl firstUrl { QUrl::fromUserInput("https://viper-browser.com") }, firstUrlRequested { QUrl::fromUserInput("viper-browser.com") };
        QDateTime firstDate  = QDateTime::currentDateTime().addDays(-1);
        historyStore->addVisit(firstUrl, QLatin1String("Viper Browser"), firstDate, firstUrlRequested, true);

        QUrl secondUrl { QUrl::fromUserInput("https://a.datacenter.website.net/landing") }, secondUrlRequested { QUrl::fromUserInput("website.net") };
        historyStore->addVisit(secondUrl, QLatin1String("Some Website"), QDateTime::currentDateTime(), secondUrlRequested, false);

        std::vector<URLRecord> records = historyStore->getHistoryFrom(firstDate);

        const URLRecord &firstRecord = records.at(0);
        QCOMPARE(firstRecord.getUrl(), firstUrl);
        QCOMPARE(firstRecord.getLastVisit(), firstDate);

        QCOMPARE(records.at(1).getUrl(), secondUrlRequested);
    }

    /*
     * todo: test cases for:

    /// Returns a queue of recently visited items, with the most recent visits being at the front of the queue
    std::deque<HistoryEntry> getRecentItems();

    /// Fetches the set of most frequently visited web pages, up to the given limit. This is used to
    /// determine which web pages' thumbnails to retrieve for the "New Tab" page
    std::vector<WebPageInformation> loadMostVisitedEntries(int limit = 10);
    */

private:
    /// Bookmark database file used for testing
    QString m_dbFile;
};

QTEST_APPLESS_MAIN(HistoryStoreTest)

#include "HistoryStoreTest.moc"
