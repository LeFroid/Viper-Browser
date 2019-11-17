#ifndef _SQLITE_DATABASE_H_
#define _SQLITE_DATABASE_H_

#include <string>

struct sqlite3;

namespace sqlite
{

class PreparedStatement;

/**
 * @class Database
 * @brief Point of entry for the library's wrapper functionality.
 *        This class manages the database connection, as well as
 *        execution and preparation of SQL statements.
 */
class Database
{
public:
    Database() = delete;
    Database(const Database&) = delete;
    Database &operator=(const Database&) = delete;

    /// Constructs the database with a given database file.
    /// The connection is opened immediately in the constructor
    explicit Database(const std::string &fileName);

    /// Closes the database connection
    ~Database();

    /// Attempts to begin a manual SQLite transaction, returning true on success, false otherwise
    bool beginTransaction();

    /// Attempts to commit a manual SQLite transaction, returning true on success, false otherwise
    bool commitTransaction();

    /// Attempts to revert a manual SQLite transaction, returning true on success, false otherwise
    bool rollbackTransaction();

    /// Executes the given statement, returning true on success, false otherwise
    bool execute(const std::string &sql);

    /// Executes the given statement, returning true on success, false otherwise
    bool execute(const char *sql);

    /// Returns the last error message, or an empty string if no errors have occurred
    const std::string &getLastError() const;
    
    /// Returns true if the connection is open and in a valid state, otherwise returns false.
    bool isValid() const;

    /**
     * @brief Prepares the given SQL statement
     * @param sql The SQL string to be prepared
     * @return Prepared statement object
     */
    PreparedStatement prepare(const std::string &sql) const;

    /**
     * @brief Prepares the given SQL statement
     * @param sql Pointer to the SQL string in UTF-8 format
     * @param nByte Length of the string, in bytes, including the null terminator ('\0')
     * @return Prepared statement object
     */
    PreparedStatement prepare(const char *sql, int nByte) const;

private:
    /// Pointer to the database connection
    sqlite3 *m_handle;

    /// Flag representing the validity of the connection
    bool m_isHandleValid;

    /// Contains any error message set from the last failing call to execute(const char*)
    std::string m_lastError;
};

}

#endif // _SQLITE_DATABASE_H_

