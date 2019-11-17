#ifndef DATABASE_BINDINGS_QT_SQLITE_H_
#define DATABASE_BINDINGS_QT_SQLITE_H_

#include "SQLiteWrapper.h"

#include <QDateTime>
#include <QString>
#include <QUrl>

/// Bindings of native Qt types to the sqlite wrapper

sqlite::PreparedStatement &operator<<(sqlite::PreparedStatement &stmt, const QDateTime &input);
sqlite::PreparedStatement &operator<<(sqlite::PreparedStatement &stmt, const QString &input);
sqlite::PreparedStatement &operator<<(sqlite::PreparedStatement &stmt, const QUrl &input);
sqlite::PreparedStatement &operator<<(sqlite::PreparedStatement &stmt, const QByteArray &input);

sqlite::PreparedStatement &operator>>(sqlite::PreparedStatement &stmt, QDateTime &output);
sqlite::PreparedStatement &operator>>(sqlite::PreparedStatement &stmt, QString &output);
sqlite::PreparedStatement &operator>>(sqlite::PreparedStatement &stmt, QUrl &output);
sqlite::PreparedStatement &operator>>(sqlite::PreparedStatement &stmt, QByteArray &output);

#endif // DATABASE_BINDINGS_QT_SQLITE_H_
