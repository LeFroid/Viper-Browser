#ifndef EXTSTORAGE_H
#define EXTSTORAGE_H

#include "DatabaseWorker.h"

#include <map>
#include <mutex>

#include <QMap>
#include <QMetaType>
#include <QObject>
#include <QStringList>
#include <QVariant>

/**
 * @class ExtStorage
 * @brief Allows browser extensions to store and retrieve data, in a similar manner as with the Web Storage API
 */
class ExtStorage : public QObject, private DatabaseWorker
{
    friend class DatabaseFactory;

    enum class Statement
    {
        GetValue,
        SetValue,
        DeleteKey,
        GetSimilarKeys
    };

    Q_OBJECT

public:
    /// Constructs the extension storage object, given the path of the storage file, the name of the database, and an
    /// optional pointer to the parent object
    explicit ExtStorage(const QString &dbFile, QObject *parent = nullptr);

    /// Extension storage destructor
    virtual ~ExtStorage();

public Q_SLOTS:
    /**
     * @brief Searches the caller's storage region
     * @param extUID Unique identifier of the calling extension
     * @param keys JSON-equivalent of an object with keys to be fetched, and default values to be returned if items are not found
     * @return JSON-equivalent of an object with the requested key-value pairs
     */
    QVariantMap getResult(const QString &extUID, const QVariantMap &keys);
    //QVariantMap getResult(const QString &extUID, const QStringList &keys);
    //QVariantMap getResult(const QString &extUID, const QString &key);

    /**
     * @brief Searches the caller's storage region for an item with the given key
     * @param extUID Unique identifier of the caller
     * @param key Name of the key
     * @return The value associated with the key for that extension, or a null QVariant if the key was not found
     */
    QVariant getItem(const QString &extUID, const QString &key);

    /**
     * @brief Inserts or updates the key-value pair in storage for the caller
     * @param extUID Unique identifier of the caller
     * @param key Name of the key
     * @param value Value to be associated with the key
     */
    void setItem(const QString &extUID, const QString &key, const QVariant &value);

    /**
     * @brief Removes the key-value pair from an extension's storage
     * @param extUID Unique identifier of the caller
     * @param key Name of the key
     */
    void removeItem(const QString &extUID, const QString &key);

    /**
     * @brief Returns a list of the keys associated with a given extension
     * @param extUID Unique identifier of the caller
     * @return List of keys associated with the extension
     */
    QVariantList listKeys(const QString &extUID);

protected:
    /// Returns true if the extension database contains the table structure(s) needed for it to function properly,
    /// false if else.
    bool hasProperStructure() override;

    /// Sets initial table structure in the database
    void setup() override {}

    /// Loads records from the database
    void load() override {}

private:
    /// Map of prepared statements
    std::map<Statement, sqlite::PreparedStatement> m_statements;

    /// Mutex
    std::mutex m_mutex;
};

#endif // EXTSTORAGE_H
