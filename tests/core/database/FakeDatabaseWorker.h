#ifndef FAKEDATABASEWORKER_H
#define FAKEDATABASEWORKER_H

#include "DatabaseWorker.h"
#include <string>
#include <vector>

/**
 * @class FakeDatabaseWorker
 * @brief Implementation of the \ref DatabaseWorker class, used for testing
 */
class FakeDatabaseWorker final : public DatabaseWorker
{
    friend class DatabaseFactory;
    friend class DatabaseWorkerTest;

public:
    /**
     * @brief FakeDatabaseWorker Constructs an object that interacts with a SQLite database
     * @param dbFile Full path of the database file
     * @param dbName Name of the database. If empty, will be the application's default database
     */
    explicit FakeDatabaseWorker(const QString &dbFile);

    /// Closes the database connection
    ~FakeDatabaseWorker();

    /// Returns a reference to the database connection
    sqlite::Database &getHandle();

    /// Returns a list of each name in the Information table
    const std::vector<std::string> &getEntries() const;

    /// Sets the list of names that will be stored in the Information table
    void setEntries(std::vector<std::string> entries);

protected:
    /// Returns true if the database contains the table structure(s) needed for it to function properly, false if else.
    bool hasProperStructure() override;

    /// Sets initial table structure(s) of the database
    void setup() override;

    /// Saves information to the database - called in destructor
    void save();

    /// Loads records from the database
    void load() override;

private:
    /// Contains entries of strings corresponding to the 'name' column of the Information table
    std::vector<std::string> m_entries;
};

#endif // FAKEDATABASEWORKER_H
