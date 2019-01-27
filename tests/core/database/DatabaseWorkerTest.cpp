#include "DatabaseFactory.h"
#include "FakeDatabaseWorker.h"

#include <algorithm>
#include <QFile>
#include <QSqlDatabase>
#include <QSqlQuery>
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
    QSqlDatabase::removeDatabase(QLatin1String("TestDB"));
}

void DatabaseWorkerTest::testCreationAndSetupOfDatabase()
{
    QVERIFY2(!QFile::exists(m_dbFile), "Database file should not exist before creating the database worker!");

    auto testDatabase = DatabaseFactory::createWorker<FakeDatabaseWorker>(m_dbFile);

    QVERIFY2(QFile::exists(m_dbFile), "Database file does not exist after creating the database worker!");

    auto &dbHandle = testDatabase->getHandle();
    QVERIFY2(dbHandle.isOpen(), "Database handle is not open after creating the worker!");
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

    QSqlQuery query(dbHandle);
    QVERIFY2(query.exec(QLatin1String("SELECT COUNT(id) FROM Information")), "SQL SELECT COUNT should execute successfully");
    QVERIFY2(query.first(), "SQL SELECT COUNT should execute successfully");

    bool ok = false;
    int count = query.value(0).toInt(&ok);
    QVERIFY2(ok, "Should be able to retrieve result from SELECT COUNT query and assign to integer variable");
    QCOMPARE(count, static_cast<int>(records.size()));

    QVERIFY2(query.exec(QLatin1String("SELECT name FROM Information")), "SQL SELECT name should execute successfully");
    while (query.next())
    {
        std::string name = query.value(0).toString().toStdString();
        auto it = std::find(records.begin(), records.end(), name);
        QVERIFY2(it != records.end(), "Name in database should correspond to one of the known records");
    }
}

QTEST_APPLESS_MAIN(DatabaseWorkerTest)

#include "DatabaseWorkerTest.moc"
