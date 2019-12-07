#include "sqlite3.h"

#include "internal/implementation.h"
#include "Badge.h"
#include "Database.h"
#include "PreparedStatement.h"

#include <iostream>

namespace sqlite
{

Database::Database(const std::string &fileName) :
    m_handle{nullptr},
    m_isHandleValid{false},
    m_lastError{}
{
    internal::Implementation::instance().init();

    if (sqlite3_open_v2(fileName.c_str(), &m_handle, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, NULL) == SQLITE_OK)
    {
        m_isHandleValid = true;
        sqlite3_busy_handler(m_handle, internal::busyHandler, nullptr); 
    }
}

Database::~Database()
{
    if (m_isHandleValid && m_handle != nullptr)
    {
        sqlite3_close_v2(m_handle);
    }
}

bool Database::isValid() const
{
    return m_isHandleValid && m_handle != nullptr;
}

bool Database::beginTransaction()
{
    static constexpr char beginTransactionSql[] = "BEGIN TRANSACTION";
    return execute(beginTransactionSql);
}

bool Database::commitTransaction()
{
    static constexpr char commitTransactionSql[] = "COMMIT";
    return execute(commitTransactionSql);
}

bool Database::rollbackTransaction()
{
    static constexpr char rollbackTransactionSql[] = "ROLLBACK";
    return execute(rollbackTransactionSql);
}

bool Database::execute(const std::string &sql)
{
    return execute(sql.c_str());
}

bool Database::execute(const char *sql)
{
    if (!isValid())
        return false;

    char *errorMsg;
    const int result = sqlite3_exec(m_handle, sql, NULL, NULL, &errorMsg);
    if (result != SQLITE_OK)
    {
        m_lastError = std::string(errorMsg);
        sqlite3_free(errorMsg);
        return false;
    }

    return true;
}

const std::string &Database::getLastError() const
{
    return m_lastError;
}

PreparedStatement Database::prepare(const std::string &sql) const
{
    return PreparedStatement({}, m_handle, sql);
}

PreparedStatement Database::prepare(const char *sql, int nByte) const
{
    return PreparedStatement({}, m_handle, sql, nByte);
}

}

