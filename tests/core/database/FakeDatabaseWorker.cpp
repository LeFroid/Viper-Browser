#include "FakeDatabaseWorker.h"

#include <QSqlError>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QTest>

FakeDatabaseWorker::FakeDatabaseWorker(const QString &dbFile) :
    DatabaseWorker(dbFile, QLatin1String("TestDB")),
    m_entries()
{
}

FakeDatabaseWorker::~FakeDatabaseWorker()
{
    save();
}

QSqlDatabase &FakeDatabaseWorker::getHandle()
{
    return m_database;
}

const std::vector<std::string> &FakeDatabaseWorker::getEntries() const
{
    return m_entries;
}

void FakeDatabaseWorker::setEntries(std::vector<std::string> entries)
{
    m_entries = entries;
}

bool FakeDatabaseWorker::hasProperStructure()
{
    return hasTable(QLatin1String("Information"));
}

void FakeDatabaseWorker::setup()
{
    // Setup table structures
    QSqlQuery query(m_database);
    QVERIFY(query.exec(QLatin1String("CREATE TABLE IF NOT EXISTS Information(id INTEGER PRIMARY KEY, name TEXT NOT NULL)")));
}

void FakeDatabaseWorker::save()
{
    QVERIFY(exec(QLatin1String("DELETE FROM Information")));

    QSqlQuery query(m_database);
    QVERIFY(query.prepare(QLatin1String("INSERT INTO Information(name) VALUES(:name)")));

    for (const auto &name : m_entries)
    {
        if (name.empty())
            continue;

        query.bindValue(QLatin1String(":name"), QVariant(QString::fromStdString(name)));
        QVERIFY(query.exec());
    }
}

void FakeDatabaseWorker::load()
{
    QSqlQuery query(m_database);
    QVERIFY(query.exec(QLatin1String("SELECT name FROM Information")));
    while (query.next())
    {
        m_entries.push_back(query.value(0).toString().toStdString());
    }
}
