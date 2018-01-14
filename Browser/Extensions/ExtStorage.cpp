#include "ExtStorage.h"

#include <QSqlDatabase>
#include <QSqlQuery>
#include <QDebug>

ExtStorage::ExtStorage(const QString &dbFile, const QString &dbName, QObject *parent) :
    QObject(parent),
    DatabaseWorker(dbFile, dbName),
    m_this(this)
{
    setObjectName("storage");
}

ExtStorage::~ExtStorage()
{
}

QVariantMap ExtStorage::getResult(const QString &extUID, const QVariantMap &keys)
{
    QSqlQuery query;
    query.prepare("SELECT value FROM ItemTable WHERE key = (:key)");

    QVariantMap results;
    for (auto it = keys.cbegin(); it != keys.cend(); ++it)
    {
        query.bindValue(":key", QString("%1%2").arg(extUID).arg(it.key()));
        if (query.exec() && query.first())
            results.insert(it.key(), query.value(0));
        else
            results.insert(it.key(), it.value());
    }
    return results;
}

bool ExtStorage::hasProperStructure()
{
    // Verify existence of ItemTable table
    return hasTable(QStringLiteral("ItemTable"));
}

void ExtStorage::setup()
{
    // Setup table structure
    if (!exec(QStringLiteral("CREATE TABLE ItemTable (key TEXT UNIQUE ON CONFLICT REPLACE, value BLOB NOT NULL ON CONFLICT FAIL)")))
        qDebug() << "[Error]: In ExtStorage::setup - unable to create item table in database.";
}
