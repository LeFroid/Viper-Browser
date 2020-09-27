#include "ExtStorage.h"

#include <QDebug>

ExtStorage::ExtStorage(const QString &dbFile, QObject *parent) :
    QObject(parent),
    DatabaseWorker(dbFile),
    m_statements(),
    m_mutex()
{
    setObjectName("storage");

    // Setup table structure
    if (!exec(QLatin1String("CREATE TABLE IF NOT EXISTS ItemTable (key TEXT UNIQUE ON CONFLICT REPLACE, value BLOB NOT NULL ON CONFLICT FAIL)")))
        qWarning() << "ExtStorage - unable to setup data table.";

    m_statements.insert(std::make_pair(Statement::GetValue,
                                       m_database.prepare(R"(SELECT value FROM ItemTable WHERE key = ?)")));
    m_statements.insert(std::make_pair(Statement::SetValue,
                                       m_database.prepare(R"(INSERT OR REPLACE INTO ItemTable(key, value) VALUES (?, ?))")));
    m_statements.insert(std::make_pair(Statement::DeleteKey,
                                       m_database.prepare(R"(DELETE FROM ItemTable WHERE key = ?)")));
    m_statements.insert(std::make_pair(Statement::GetSimilarKeys,
                                       m_database.prepare(R"(SELECT key FROM ItemTable WHERE key LIKE ?)")));
}

ExtStorage::~ExtStorage()
{
}

QVariantMap ExtStorage::getResult(const QString &extUID, const QVariantMap &keys)
{
    sqlite::PreparedStatement &stmt = m_statements.at(Statement::GetValue);
    stmt.reset();

    QVariantMap results;
    for (auto it = keys.cbegin(); it != keys.cend(); ++it)
    {
        std::string boundParam = QString("%1%2").arg(extUID, it.key()).toStdString();
        stmt.bind(0, boundParam);
        if (stmt.next())
        {
            sqlite::Blob blob;
            stmt >> blob;
            QVariant temp = QVariant(QString::fromStdString(blob.data));
            results.insert(it.key(), temp);
        }
        else
            results.insert(it.key(), it.value());
    }
    return results;
}

QVariant ExtStorage::getItem(const QString &extUID, const QString &key)
{
    QVariant result;

    sqlite::PreparedStatement &stmt = m_statements.at(Statement::GetValue);
    stmt.reset();

    std::string boundParam = QString("%1%2").arg(extUID, key).toStdString();
    stmt.bind(0, boundParam);

    if (stmt.next())
    {
        sqlite::Blob blob;
        stmt >> blob;
        result = QVariant(QString::fromStdString(blob.data));
    }

    return result;
}

void ExtStorage::setItem(const QString &extUID, const QString &key, const QVariant &value)
{
    sqlite::PreparedStatement &stmt = m_statements.at(Statement::SetValue);
    stmt.reset();

    std::string boundKey = QString("%1%2").arg(extUID, key).toStdString();
    sqlite::Blob boundVal { value.toString().toStdString() };

    stmt.bind(0, boundKey);
    stmt.bind(1, boundVal);

    if (!stmt.execute())
        qDebug() << "ExtStorage::setItem - could not update value with key name " << QString::fromStdString(boundKey);
}

void ExtStorage::removeItem(const QString &extUID, const QString &key)
{
    sqlite::PreparedStatement &stmt = m_statements.at(Statement::DeleteKey);
    stmt.reset();

    std::string boundKey = QString("%1%2").arg(extUID, key).toStdString();
    stmt.bind(0, boundKey);
    if (!stmt.execute())
        qDebug() << "ExtStorage::removeItem - could not remove key from the database. Key name: "
                 << QString::fromStdString(boundKey);
}

QVariantList ExtStorage::listKeys(const QString &extUID)
{
    sqlite::PreparedStatement &stmt = m_statements.at(Statement::GetSimilarKeys);
    stmt.reset();

    std::string boundKey = QString("%1%").arg(extUID).toStdString();
    stmt.bind(0, boundKey);

    QVariantList result;

    if (stmt.execute())
    {
        while (stmt.next())
        {
            sqlite::Blob blob;
            stmt >> blob;
            result.push_back(QVariant(QString::fromStdString(blob.data)));
        }
    }

    return result;
}

bool ExtStorage::hasProperStructure()
{
    return true;
}
