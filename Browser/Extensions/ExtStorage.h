#ifndef EXTSTORAGE_H
#define EXTSTORAGE_H

#include "DatabaseWorker.h"
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
    Q_OBJECT

    Q_PROPERTY(ExtStorage* local MEMBER m_this)
    Q_PROPERTY(ExtStorage* sync MEMBER m_this)

public:
    /// Constructs the extension storage object, given the path of the storage file, the name of the database, and an
    /// optional pointer to the parent object
    explicit ExtStorage(const QString &dbFile, const QString &dbName, QObject *parent = nullptr);

    /// Extension storage destructor
    virtual ~ExtStorage();

public slots:
    /**
     * @brief Searches the caller's storage region
     * @param extUID Unique identifier of the calling extension
     * @param keys JSON-equivalent of an object with keys to be fetched, and default values to be returned if items are not found
     * @return JSON-equivalent of an object with the requested key-value pairs
     */
    QVariantMap getResult(const QString &extUID, const QVariantMap &keys);
    //QVariantMap getResult(const QString &extUID, const QStringList &keys);
    //QVariantMap getResult(const QString &extUID, const QString &key);

protected:
    /// Returns true if the extension database contains the table structure(s) needed for it to function properly,
    /// false if else.
    bool hasProperStructure() override;

    /// Sets initial table structure in the database
    void setup() override;

    /// Saves information to the database - called in destructor
    void save() override {}

    /// Loads records from the database
    void load() override {}

private:
    ExtStorage *m_this;
};

Q_DECLARE_METATYPE(ExtStorage*)

#endif // EXTSTORAGE_H
