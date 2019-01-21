#include "ExtStorage.h"

#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QDebug>

ExtStorage::ExtStorage(const QString &dbFile, QObject *parent) :
    QObject(parent),
    DatabaseWorker(dbFile, QLatin1String("ExtStorage"))
{
    setObjectName("storage");
}

ExtStorage::~ExtStorage()
{
}

QVariantMap ExtStorage::getResult(const QString &extUID, const QVariantMap &keys)
{
    QSqlQuery query(m_database);
    query.prepare(QLatin1String("SELECT value FROM ItemTable WHERE key = (:key)"));

    QVariantMap results;
    for (auto it = keys.cbegin(); it != keys.cend(); ++it)
    {
        query.bindValue(QLatin1String(":key"), QString("%1%2").arg(extUID).arg(it.key()));
        if (query.exec() && query.first())
            results.insert(it.key(), query.value(0));
        else
            results.insert(it.key(), it.value());
    }
    return results;
}

QVariant ExtStorage::getItem(const QString &extUID, const QString &key)
{
    QVariant result;

    QSqlQuery query(m_database);
    query.prepare(QLatin1String("SELECT value FROM ItemTable WHERE key = (:key)"));
    query.bindValue(QLatin1String(":key"), QString("%1%2").arg(extUID).arg(key));
    if (query.exec() && query.first())
        result = query.value(0);

    return result;
}

void ExtStorage::setItem(const QString &extUID, const QString &key, const QVariant &value)
{
    QSqlQuery query(m_database);
    query.prepare(QLatin1String("INSERT OR REPLACE INTO ItemTable(key, value) VALUES (:key, :value)"));
    query.bindValue(QLatin1String(":key"), QString("%1%2").arg(extUID).arg(key));
    query.bindValue(QLatin1String(":value"), value);
    if (!query.exec())
        qDebug() << "ExtStorage::setItem - could not update value in the database. Error message: "
                 << query.lastError().text();
}

void ExtStorage::removeItem(const QString &extUID, const QString &key)
{
    QSqlQuery query(m_database);
    query.prepare(QLatin1String("DELETE FROM ItemTable WHERE KEY = (:key)"));
    query.bindValue(QLatin1String(":key"), QString("%1%2").arg(extUID).arg(key));
    if (!query.exec())
        qDebug() << "ExtStorage::removeItem - could not remove key from the database. Error message: "
                 << query.lastError().text();
}

QVariantList ExtStorage::listKeys(const QString &extUID)
{
    QVariantList result;

    QSqlQuery query(m_database);
    query.prepare(QLatin1String("SELECT key FROM ItemTable WHERE key LIKE (:key)"));
    query.bindValue(QLatin1String(":key"), QString("%1%").arg(extUID));
    if (query.exec())
    {
        while (query.next())
            result.push_back(query.value(0));
    }

    return result;
}

bool ExtStorage::hasProperStructure()
{
    // Verify existence of ItemTable table
    return hasTable(QStringLiteral("ItemTable"));
}

void ExtStorage::setup()
{
    // Setup table structure
    if (!exec(QLatin1String("CREATE TABLE ItemTable (key TEXT UNIQUE ON CONFLICT REPLACE, value BLOB NOT NULL ON CONFLICT FAIL)")))
        qDebug() << "[Error]: In ExtStorage::setup - unable to create item table in database.";
}
