#include "DatabaseFactory.h"
#include "HistoryManager.h"
#include "HistoryStore.h"
#include "ServiceLocator.h"

#include <QFile>
#include <QObject>
#include <QString>
#include <QTest>

/// Test cases for the \ref HistoryManager class
class HistoryManagerTest : public QObject
{
    Q_OBJECT

public:
    HistoryManagerTest() :
        QObject(nullptr),
        m_dbFile(QLatin1String("HistoryManagerTest.db"))
    {
    }

private slots:
    /// Called before any tests are executed
    void initTestCase()
    {
        if (QFile::exists(m_dbFile))
            QFile::remove(m_dbFile);
    }

    /// Called before every test function. Instantiates the history manager since this
    /// requires a fair amount of code and would otherwise be too repetitive.
    void init()
    {

    }

    /// Called after every test function. This closes the DB connection and deallocates / clears
    /// the HistoryStore instance
    void cleanup()
    {
        if (QFile::exists(m_dbFile))
            QFile::remove(m_dbFile);
        QSqlDatabase::removeDatabase(QLatin1String("HistoryDB"));
    }

    /// Tests that entries can be added to the history store
    void testAddVisits()
    {
        /*
        DatabaseTaskScheduler taskScheduler;
        taskScheduler.addWorker("HistoryStore", std::bind(DatabaseFactory::createDBWorker<HistoryStore>, "HistoryManagerTest.db"));

        ViperServiceLocator serviceLocator;

        HistoryManager historyManager(serviceLocator, taskScheduler);

        taskScheduler.run();

        QUrl firstUrl { QUrl::fromUserInput("https://viper-browser.com") }, firstUrlRequested { QUrl::fromUserInput("viper-browser.com") };
        QDateTime firstDate  = QDateTime::currentDateTime();
        historyManager.addVisit(firstUrl, QLatin1String("Viper Browser"), firstDate, firstUrlRequested);

        QUrl secondUrl { QUrl::fromUserInput("https://a.datacenter.website.net/landing") }, secondUrlRequested { QUrl::fromUserInput("website.net") };
        historyManager.addVisit(secondUrl, QLatin1String("Some Website"), QDateTime::currentDateTime(), secondUrlRequested);

        QVERIFY(historyManager.contains(firstUrl));
        QVERIFY(historyManager.contains(secondUrl));
        QVERIFY(historyManager.contains(secondUrlRequested));

        HistoryEntry entry = historyManager.getEntry(firstUrl);
        QCOMPARE(entry.URL, firstUrl);
        QCOMPARE(entry.Title, QLatin1String("Viper Browser"));
        QCOMPARE(entry.NumVisits, 1);
        QCOMPARE(entry.LastVisit, firstDate);*/
    }

    // Instantiating the history manager:
    //DatabaseTaskScheduler taskScheduler;
    //taskScheduler.addWorker("HistoryStore", std::bind(DatabaseFactory::createDBWorker<HistoryStore>, "HistoryManagerTest.db"));
    //ViperServiceLocator serviceLocator;
    //HistoryManager *historyManager = new HistoryManager(serviceLocator, taskScheduler);
    //taskScheduler.run();
    /*explicit HistoryManager(const ViperServiceLocator &serviceLocator, DatabaseTaskScheduler &taskScheduler);

    /// Returns a const_iterator to the first element in the history hash map
    const_iterator begin() const { return m_historyItems.cbegin(); }

    /// Returns a const_iterator to the end of the history hash map
    const_iterator end() const { return m_historyItems.cend(); }

    /// Clears all browsing history
    void clearAllHistory();

    /// Clears history stored from the given start time to the present
    void clearHistoryFrom(const QDateTime &start);

    /// Clears history within the given {start,end} date-time pair
    void clearHistoryInRange(std::pair<QDateTime, QDateTime> range);

    /// Adds an entry to the history data store, given the URL, page title, time of visit, and the requested URL
    void addVisit(const QUrl &url, const QString &title, const QDateTime &visitTime, const QUrl &requestedUrl);

    /// Loads a list of all \ref URLRecord visited between the given start date and end dates, passing them on to
    /// the callback once the data has been fetched
    void getHistoryBetween(const QDateTime &startDate, const QDateTime &endDate, std::function<void(std::vector<URLRecord>)> callback);

    /// Loads a list of all \ref URLRecord visited from the given start date to the present, passing them on to
    /// the callback once the data has been fetched
    void getHistoryFrom(const QDateTime &startDate, std::function<void(std::vector<URLRecord>)> callback);

    /// Returns true if the history contains the given url, false if else. Will return
    /// false if private browsing mode is enabled
    bool historyContains(const QString &url) const;

    /// Returns a history record corresponding to the given URL, or an empty record if it was not found in the
    /// database
    HistoryEntry getEntry(const QUrl &url) const;

    /// Returns a queue of recently visited items, with the most recent visits being at the front of the queue
    const std::deque<HistoryEntry> &getRecentItems() const { return m_recentItems; }

    /// Returns a list of all visited URLs
    QList<QString> getVisitedURLs() const { return m_historyItems.keys(); }

    /// Fetches the number of times the host was visited, passing it to the given callback when the
    /// result has been retrieved
    void getTimesVisitedHost(const QUrl &host, std::function<void(int)> callback);

    /// Returns the number of times that the given URL has been visited
    int getTimesVisited(const QUrl &url) const;

    /// Returns the history manager's storage policy
    HistoryStoragePolicy getStoragePolicy() const;

    /// Sets the policy to be followed for storing browsing history
    void setStoragePolicy(HistoryStoragePolicy policy);

    /// Fetches the set of most frequently visited web pages, up to the given limit. This is used to
    /// determine which web pages' thumbnails to retrieve for the "New Tab" page
    void loadMostVisitedEntries(int limit, std::function<void(std::vector<WebPageInformation>)> callback);*/

private:
    /// Database file name used for tests
    QString m_dbFile;
};

QTEST_APPLESS_MAIN(HistoryManagerTest)

#include "HistoryManagerTest.moc"

