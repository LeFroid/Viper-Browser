#include "Database.h"
#include "PreparedStatement.h"

namespace sqlite
{

PreparedStatement::PreparedStatement(Badge<Database>, sqlite3 *db, const std::string &sql) :
    m_handle{nullptr},
    m_state{State::NotReady},
    m_colIdx{0},
    m_numCols{0}
{
    const int status = sqlite3_prepare_v2(db, sql.c_str(), 1 + static_cast<int>(sql.size()),
            &m_handle, NULL);

    if (status == SQLITE_OK)
        m_state = State::Ready;
    else
        m_handle = nullptr;
}

PreparedStatement::PreparedStatement(Badge<Database>, sqlite3 *db, const char *sql, int nByte) :
    m_handle{nullptr},
    m_state{State::NotReady},
    m_colIdx{0},
    m_numCols{0}
{
    const int status = sqlite3_prepare_v2(db, sql, nByte, &m_handle, NULL);

    if (status == SQLITE_OK)
        m_state = State::Ready;
    else
        m_handle = nullptr;
}

PreparedStatement::~PreparedStatement()
{
    if (m_handle != nullptr)
    {
        sqlite3_finalize(m_handle);
        m_handle = nullptr;
    }
}

bool PreparedStatement::execute()
{
    if (m_handle == nullptr || m_state != State::Ready)
        return false;

    const int status = sqlite3_step(m_handle);
    if (status == SQLITE_DONE)
    {
        sqlite3_reset(m_handle);
        m_colIdx = 0;
        return true;
    }
    else if (status == SQLITE_ROW)
    {
        m_state = State::FirstRow;
        m_colIdx = 0;
        m_numCols = sqlite3_column_count(m_handle);
        return true;
    }

    m_state = State::Error;
    ///TODO: fetch error message
    return false;
}

bool PreparedStatement::next()
{
    if (m_handle == nullptr)
        return false;

    if (m_state == State::FirstRow) 
    {
        m_state = State::InProgress;
        return true;
    }
    else if (m_state == State::Ready || m_state == State::InProgress)
    {
        const int status = sqlite3_step(m_handle);
        if (status == SQLITE_ROW)
        {
            if (m_state == State::Ready)
            {
                m_state = State::InProgress;
                m_numCols = sqlite3_column_count(m_handle);
            }
            m_colIdx = 0;
            return true;
        }

        sqlite3_clear_bindings(m_handle);
        sqlite3_reset(m_handle);
        m_colIdx = 0;
        m_numCols = 0;
        m_state = State::Ready;
        return false;
    }

    return false;
}

void PreparedStatement::reset()
{
    if (m_handle == nullptr)
        return;

    sqlite3_clear_bindings(m_handle);
    sqlite3_reset(m_handle);

    m_state = State::Ready;
    m_colIdx = 0;
    m_numCols = 0;
}

}
