#include "database/bindings/QtSQLite.h"

sqlite::PreparedStatement &operator>>(sqlite::PreparedStatement &stmt, QDateTime &output)
{
    int64_t temp;
    stmt >> temp;
    output = QDateTime::fromMSecsSinceEpoch(temp);
    return stmt;
}

sqlite::PreparedStatement &operator>>(sqlite::PreparedStatement &stmt, QString &output)
{
    std::string temp;
    stmt >> temp;
    output = QString::fromStdString(temp);
    return stmt;
}

sqlite::PreparedStatement &operator>>(sqlite::PreparedStatement &stmt, QUrl &output)
{
    std::string temp;
    stmt >> temp;
    output = QUrl(QString::fromStdString(temp));
    return stmt;
}
