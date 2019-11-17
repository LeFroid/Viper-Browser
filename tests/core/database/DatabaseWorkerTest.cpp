#include "DatabaseFactory.h"
#include "FakeDatabaseWorker.h"

#include <algorithm>
#include <QFile>
#include <QString>
#include <QTest>

/// Tests an implementation of the DatabaseWorker and DatabaseFactory classes
class DatabaseWorkerTest : public QObject
{
    Q_OBJECT

public:
    DatabaseWorkerTest() : QObject(), m_dbFile("Test.db") {}

private slots:
    void cleanup();

    void testCreationAndSetupOfDatabase();

    void testSaveAndRetrieveRecordsFromDatabase();

private:
    QString m_dbFile;
};

void DatabaseWorkerTest::cleanup()
{
    if (!QFile::exists(m_dbFile))
        return;

    QFile::remove(m_dbFile);
}

void DatabaseWorkerTest::testCreationAndSetupOfDatabase()
{
    QVERIFY2(!QFile::exists(m_dbFile), "Database file should not exist before creating the database worker!");

    auto testDatabase = DatabaseFactory::createWorker<FakeDatabaseWorker>(m_dbFile);

    QVERIFY2(QFile::exists(m_dbFile), "Database file does not exist after creating the database worker!");

    auto &dbHandle = testDatabase->getHandle();
    QVERIFY2(dbHandle.isValid(), "Database handle is not valid!");

    QVERIFY2(testDatabase->hasProperStructure(), "Table structure should have been set when instantiating the database worker");
}

void DatabaseWorkerTest::testSaveAndRetrieveRecordsFromDatabase()
{
    auto testDatabase = DatabaseFactory::createWorker<FakeDatabaseWorker>(m_dbFile);

    std::vector<std::string> records { "Tom", "Dick", "Harry" };
    testDatabase->setEntries(records);
    testDatabase->save();

    auto &dbHandle = testDatabase->getHandle();

    auto query = dbHandle.prepare(R"(SELECT COUNT(id) FROM Information)");
    QVERIFY2(query.execute(), "SQL SELECT COUNT should execute successfully");
    QVERIFY2(query.next(), "SQL SELECT COUNT should execute successfully");

    int count = -1;
    query >> count;
    QCOMPARE(count, static_cast<int>(records.size()));

    query = dbHandle.prepare(R"(SELECT name FROM Information)");
    QVERIFY2(query.execute(), "SQL SELECT name should execute successfully");
    while (query.next())
    {
        std::string name;
        query >> name;
        auto it = std::find(records.begin(), records.end(), name);
        QVERIFY2(it != records.end(), "Name in database should correspond to one of the known records");
    }
}

QTEST_APPLESS_MAIN(DatabaseWorkerTest)

#include "DatabaseWorkerTest.moc"
