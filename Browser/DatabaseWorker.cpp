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
    if (!exec("PRAGMA journal_mode=WAL"))
        qDebug() << "[Error]: In DatabaseWorker constructor - could not set journal mode.";
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
