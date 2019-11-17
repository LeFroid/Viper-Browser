#include "FakeDatabaseWorker.h"

#include <QTest>

FakeDatabaseWorker::FakeDatabaseWorker(const QString &dbFile) :
    DatabaseWorker(dbFile),
    m_entries()
{
}

FakeDatabaseWorker::~FakeDatabaseWorker()
{
    save();
}

sqlite::Database &FakeDatabaseWorker::getHandle()
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
    QVERIFY(exec(QLatin1String("CREATE TABLE IF NOT EXISTS Information(id INTEGER PRIMARY KEY, name TEXT NOT NULL)")));
}

void FakeDatabaseWorker::save()
{
    QVERIFY(exec(QLatin1String("DELETE FROM Information")));

    auto stmt = m_database.prepare(R"(INSERT INTO Information(name) VALUES (?))");
    for (const auto &name : m_entries)
    {
        if (name.empty())
            continue;

        stmt << name;
        QVERIFY(stmt.execute());
        stmt.reset();
    }
}

void FakeDatabaseWorker::load()
{
    auto query = m_database.prepare(R"(SELECT name FROM Information)");
    while (query.next())
    {
        std::string name;
        query >> name;
        m_entries.push_back(name);
    }
}
