#include "DatabaseWorker.h"

#include <QDebug>
#include <QSqlQuery>
#include <QSqlError>

DatabaseWorker::DatabaseWorker(const QString &dbFile, const QString &dbName) :
    m_database(QSqlDatabase::addDatabase("QSQLITE", dbName))
{
    m_database.setDatabaseName(dbFile);
    if (!m_database.open())
        qDebug() << "[Error]: Unable to open database " << dbFile;

    // Turn synchronous setting off
    if (!exec(QStringLiteral("PRAGMA journal_mode=WAL")))
        qDebug() << "[Error]: In DatabaseWorker constructor - could not set journal mode.";

    // Foreign keys
    if (!exec(QStringLiteral("PRAGMA foreign_keys=ON")))
        qDebug() << "In DatabaseWorker constructor - could not enable foreign keys.";
}

DatabaseWorker::~DatabaseWorker()
{
    m_database.close();
}

bool DatabaseWorker::exec(const QString &queryString)
{
    QSqlQuery query(m_database);
    return query.exec(queryString);
}

bool DatabaseWorker::hasTable(const QString &tableName)
{
    QSqlQuery query(m_database);

    query.prepare(QStringLiteral("SELECT COUNT(*) FROM sqlite_master WHERE type= (:type) AND name = (:name)"));
    query.bindValue(QStringLiteral(":type"), QStringLiteral("table"));
    query.bindValue(QStringLiteral(":name"), tableName);
    if (query.exec())
        return (query.first() && query.value(0).toInt() == 1);

    return false;
}
