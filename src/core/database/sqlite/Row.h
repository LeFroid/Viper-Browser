#ifndef _SQLITE_ROW_H_
#define _SQLITE_ROW_H_

namespace sqlite
{

class PreparedStatement;

/**
 * @struct Row
 * @brief Provides an interface for SQLite API consumers that would like 
 *        to serialize or deserialize a data type in the same manner as
 *        a primitive type. This enables the use of stream operators
 *        to or from a \ref PreparedStatement , as well as the ability
 *        to call PreparedStatement.read(Row), PreparedStatement.write(Row),
 *        and PreparedStatement.bind(index, Row)
 */
struct Row
{
    // Serialize the data structure into the prepared statement's bound parameters
    virtual void marshal(PreparedStatement &stmt) const = 0;

    // Deserialize the result of a query into the data structure
    virtual void unmarshal(PreparedStatement &stmt) = 0;
};

}

#endif // _SQLITE_ROW_H_

