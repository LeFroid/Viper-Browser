#include "DatabaseFactory.h"
#include "HistoryManager.h"
#include "HistoryStore.h"
#include "ServiceLocator.h"

#include <QDateTime>
#include <QFile>
#include <QObject>
#include <QSignalSpy>
#include <QString>
#include <QTest>

/// Test cases for the \ref HistoryManager class
class HistoryManagerTest : public QObject
{
    Q_OBJECT

public:
    HistoryManagerTest() :
        QObject(nullptr),
        m_dbFile(QLatin1String("HistoryManagerTest.db")),
        m_historyManager(nullptr)
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
        if (m_historyManager)
        {
            delete m_historyManager;
            m_historyManager = nullptr;
        }

        if (QFile::exists(m_dbFile))
            QFile::remove(m_dbFile);
    }

    /// Tests that entries can be added to the history store
    void testAddVisits()
    {
        DatabaseTaskScheduler taskScheduler;
        taskScheduler.addWorker("HistoryStore", std::bind(DatabaseFactory::createDBWorker<HistoryStore>, "HistoryManagerTest.db"));

        ViperServiceLocator serviceLocator;

        m_historyManager = new HistoryManager(serviceLocator, taskScheduler);

        taskScheduler.run();

        QUrl firstUrl { QUrl::fromUserInput("https://viper-browser.com") }, firstUrlRequested { QUrl::fromUserInput("viper-browser.com") };
        QDateTime firstDate  = QDateTime::currentDateTime();
        m_historyManager->addVisit(firstUrl, QLatin1String("Viper Browser"), firstDate, firstUrlRequested, true);

        QUrl secondUrl { QUrl::fromUserInput("https://a.datacenter.website.net/landing") }, secondUrlRequested { QUrl::fromUserInput("website.net") };
        m_historyManager->addVisit(secondUrl, QLatin1String("Some Website"), QDateTime::currentDateTime(), secondUrlRequested, false);

        auto verifyTrueCB = [](bool result){
            QVERIFY(result);
        };
        m_historyManager->contains(firstUrl, verifyTrueCB);
        m_historyManager->contains(secondUrl, verifyTrueCB);
        m_historyManager->contains(secondUrlRequested, verifyTrueCB);

        HistoryEntry entry = m_historyManager->getEntry(firstUrl);
        QCOMPARE(entry.URL, firstUrl);
        QCOMPARE(entry.Title, QLatin1String("Viper Browser"));
        QCOMPARE(entry.NumVisits, 1);
        QCOMPARE(entry.LastVisit, firstDate);

        QTest::qWait(1500);
    }

    /// Tests that the browsing history can be cleared
    void testClearingHistory()
    {
        DatabaseTaskScheduler taskScheduler;
        taskScheduler.addWorker("HistoryStore", std::bind(DatabaseFactory::createDBWorker<HistoryStore>, "HistoryManagerTest.db"));

        ViperServiceLocator serviceLocator;

        m_historyManager = new HistoryManager(serviceLocator, taskScheduler);

        taskScheduler.run();

        auto verifyTrueCB = [](bool result){
            QVERIFY(result);
        };
        auto verifyFalseCB = [](bool result){
            QVERIFY(!result);
        };

        // Let initialization routine complete
        QTest::qWait(1500);

        QUrl firstUrl { QUrl::fromUserInput("https://a.datacenter.website.net/landing") }, firstUrlRequested { QUrl::fromUserInput("website.net") };
        m_historyManager->addVisit(firstUrl, QLatin1String("Some Website"), QDateTime::currentDateTime(), firstUrlRequested, true);

        m_historyManager->contains(firstUrl, verifyTrueCB);
        m_historyManager->contains(firstUrlRequested, verifyTrueCB);

        m_historyManager->clearAllHistory();

        m_historyManager->contains(firstUrl, verifyFalseCB);
        m_historyManager->contains(firstUrlRequested, verifyFalseCB);
        //QVERIFY2(!m_historyManager->contains(firstUrl), "HistoryManager::clearAllHistory did not remove the entry");
        //QVERIFY2(!m_historyManager->contains(firstUrlRequested), "HistoryManager::clearAllHistory did not remove the entry");

        QDateTime firstDate  = QDateTime::currentDateTime().addDays(-5);
        QDateTime secondDate = QDateTime::currentDateTime();
        const QUrl secondUrl { QUrl::fromUserInput("https://viper-browser.com") }, secondUrlRequested { QUrl::fromUserInput("viper-browser.com") };

        m_historyManager->addVisit(firstUrl, QLatin1String("Some Website"), firstDate, firstUrlRequested, true);
        m_historyManager->addVisit(secondUrl, QLatin1String("Viper Browser"), secondDate, secondUrlRequested, true);

        m_historyManager->contains(firstUrl, verifyTrueCB);
        m_historyManager->contains(secondUrl, verifyTrueCB);
        //QVERIFY(m_historyManager->contains(firstUrl));
        //QVERIFY(m_historyManager->contains(secondUrl));

        // Use signal spy to make sure the clear history call makes its way to the history store
        QSignalSpy spy(m_historyManager, &HistoryManager::historyCleared);
        m_historyManager->clearHistoryInRange(std::make_pair(firstDate, QDateTime::currentDateTime().addDays(-1)));

        QVERIFY(spy.wait(5500));
        QCOMPARE(spy.count(), 1);

        m_historyManager->contains(firstUrl, verifyFalseCB);
        m_historyManager->contains(secondUrl, verifyTrueCB);
        //QVERIFY2(!m_historyManager->contains(firstUrl), "HistoryManager::clearHistoryInRange did not remove the entry");
        //QVERIFY2(m_historyManager->contains(secondUrl), "HistoryManager::clearHistoryInRange removed an entry outside of the given range");

        m_historyManager->clearHistoryFrom(QDateTime::currentDateTime().addDays(-1));

        m_historyManager->contains(secondUrl, verifyFalseCB);
        //QVERIFY2(!m_historyManager->contains(secondUrl), "HistoryManager::clearHistoryFrom did not remove the entry");

        QVERIFY(spy.wait(5500));
        QCOMPARE(spy.count(), 2);
    }

    /// Tests that the reported visit count matches the expected values
    void testVisitCount()
    {
        DatabaseTaskScheduler taskScheduler;
        taskScheduler.addWorker("HistoryStore", std::bind(DatabaseFactory::createDBWorker<HistoryStore>, "HistoryManagerTest.db"));

        ViperServiceLocator serviceLocator;

        m_historyManager = new HistoryManager(serviceLocator, taskScheduler);

        taskScheduler.run();

        // Let initialization routine complete
        QTest::qWait(500);

        QUrl firstUrl { QUrl::fromUserInput("https://viper-browser.com") }, firstUrlRequested { QUrl::fromUserInput("viper-browser.com") };
        QUrl secondUrl { QUrl::fromUserInput("https://a.datacenter.website.net/landing") }, secondUrlRequested { QUrl::fromUserInput("website.net") };
        m_historyManager->addVisit(firstUrl, QLatin1String("Viper Browser"), QDateTime::currentDateTime(), firstUrlRequested, false);
        m_historyManager->addVisit(secondUrl, QLatin1String("Some Website"), QDateTime::currentDateTime(), secondUrlRequested, false);

        m_historyManager->getTimesVisitedHost(firstUrl, [](int count){
            QCOMPARE(count, 1);
        });
        m_historyManager->getTimesVisitedHost(secondUrl, [](int count){
            QCOMPARE(count, 1);
        });
        m_historyManager->getTimesVisitedHost(secondUrlRequested, [](int count){
            QCOMPARE(count, 2);
        });

        QTest::qWait(300);
    }

    /// Tests the call to getHistoryFrom(const QDateTime &startDate)
    void testGetHistoryFrom()
    {
        DatabaseTaskScheduler taskScheduler;
        taskScheduler.addWorker("HistoryStore", std::bind(DatabaseFactory::createDBWorker<HistoryStore>, "HistoryManagerTest.db"));

        ViperServiceLocator serviceLocator;

        m_historyManager = new HistoryManager(serviceLocator, taskScheduler);

        taskScheduler.run();

        // Let initialization routine complete
        QTest::qWait(500);

        QUrl firstUrl { QUrl::fromUserInput("https://viper-browser.com") }, firstUrlRequested { QUrl::fromUserInput("viper-browser.com") };
        QDateTime firstDate  = QDateTime::currentDateTime().addDays(-1);
        m_historyManager->addVisit(firstUrl, QLatin1String("Viper Browser"), firstDate, firstUrlRequested, true);

        QUrl secondUrl { QUrl::fromUserInput("https://a.datacenter.website.net/landing") }, secondUrlRequested { QUrl::fromUserInput("website.net") };
        m_historyManager->addVisit(secondUrl, QLatin1String("Some Website"), QDateTime::currentDateTime(), secondUrlRequested, true);

        m_historyManager->getHistoryFrom(firstDate, [=](std::vector<URLRecord> records){
            const URLRecord &firstRecord = records.at(0);
            QCOMPARE(firstRecord.getUrl(), firstUrl);
            QCOMPARE(firstRecord.getLastVisit(), firstDate);

            QCOMPARE(records.at(1).getUrl(), secondUrlRequested);
        });

        QTest::qWait(1500);
    }

private:
    /// Database file name used for tests
    QString m_dbFile;

    /// Points to the history manager. This is recreated on every test case.
    /// Must instantiate this on heap since its dependencies must be created before this,
    /// yet the history manager must also be the last thing to be destroyed after a test
    HistoryManager *m_historyManager;
};

QTEST_GUILESS_MAIN(HistoryManagerTest)

#include "HistoryManagerTest.moc"

