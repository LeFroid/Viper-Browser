#include "DatabaseWorker.h"

#include <QDebug>

DatabaseWorker::DatabaseWorker(const QString &dbFile) :
    m_database(dbFile.toStdString())
{
    if (!m_database.isValid())
        qWarning() << "Unable to open database " << dbFile;

    // Turn synchronous setting off
    if (!m_database.execute("PRAGMA journal_mode=WAL"))
        qWarning() << "In DatabaseWorker constructor - could not set journal mode.";

    // Foreign keys
    if (!m_database.execute("PRAGMA foreign_keys=\"1\""))
        qWarning() << "In DatabaseWorker constructor - could not enable foreign keys.";
}

DatabaseWorker::~DatabaseWorker()
{
}

bool DatabaseWorker::exec(const QString &queryString)
{
    return m_database.execute(queryString.toStdString());
}

bool DatabaseWorker::hasTable(const QString &tableName)
{
    sqlite::PreparedStatement stmt = m_database.prepare(R"(SELECT COUNT(*) FROM sqlite_master WHERE type = 'table' AND name = ?)");
    stmt << tableName;
    if (stmt.next())
    {
        int result = 0;
        stmt >> result;
        return result == 1;
    }

    return false;
}
