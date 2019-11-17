#include "database/bindings/QtSQLite.h"

sqlite::PreparedStatement &operator<<(sqlite::PreparedStatement &stmt, const QDateTime &input)
{
    int64_t temp = input.toMSecsSinceEpoch();
    stmt.read(temp, true);
    return stmt;
}

sqlite::PreparedStatement &operator<<(sqlite::PreparedStatement &stmt, const QString &input)
{
    const std::string temp = input.toStdString();
    stmt.read(temp, true);
    return stmt;
}

sqlite::PreparedStatement &operator<<(sqlite::PreparedStatement &stmt, const QUrl &input)
{
    const std::string temp = input.toString(QUrl::FullyEncoded).toStdString();
    stmt.read(temp, true);
    return stmt;
}

sqlite::PreparedStatement &operator<<(sqlite::PreparedStatement &stmt, const QByteArray &input)
{
    sqlite::Blob temp { input.toStdString() };
    stmt.read(temp, true);
    return stmt;
}

sqlite::PreparedStatement &operator>>(sqlite::PreparedStatement &stmt, QDateTime &output)
{
    int64_t temp = 0;
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

sqlite::PreparedStatement &operator>>(sqlite::PreparedStatement &stmt, QByteArray &output)
{
    sqlite::Blob temp;
    stmt >> temp;
    output = QByteArray::fromStdString(temp.data);
    return stmt;
}
