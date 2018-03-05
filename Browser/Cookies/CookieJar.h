#ifndef COOKIEJAR_H
#define COOKIEJAR_H

#include "DatabaseWorker.h"
#include <map>
#include <memory>
#include <QDateTime>
#include <QNetworkCookieJar>
#include <QSqlQuery>

/**
 * @class CookieJar
 * @brief Implements ability to store and load cookies on disk
 */
class CookieJar : public QNetworkCookieJar, private DatabaseWorker
{
    friend class CookieTableModel;
    friend class DatabaseFactory;

    Q_OBJECT

public:
    /// Constructs a cookie jar, loading information from the given cookie database file if private mode is false
    explicit CookieJar(const QString &databaseFile, QString name = QString("Cookies"), bool privateJar = false, QObject *parent = 0);

    /// Saves cookies to database before calling ~QNetworkCookieJar
    ~CookieJar();

    /// Returns true if the CookieJar contains 1 or more cookies associated with the given host, false if else
    bool hasCookiesFor(const QString &host) const;

    /// Clears cookies stored from the given start time to the present
    void clearCookiesFrom(const QDateTime &start);

    /// Clears cookies that were created any time within the {start,end} date-time range pair
    void clearCookiesInRange(std::pair<QDateTime, QDateTime> range);

    /// Emits a singal when a cookie has been added to the jar
    bool insertCookie(const QNetworkCookie &cookie) override;

    /// Deletes the given cookie if found in the cookie jar
    bool deleteCookie(const QNetworkCookie &cookie) override;

    /// Erases all cookies from memory, as well as the database
    void eraseAllCookies();

signals:
    /// Emitted when a new cookie has been added to the jar
    void cookieAdded();

    /// Emitted when all the cookies have been erased
    void cookiesRemoved();

protected:
    /// Returns true if the cookie database contains the table structure(s) needed for it to function properly,
    /// false if else.
    bool hasProperStructure() override;

    /// Sets initial table structure for storage of cookies
    void setup() override;

    /// Saves cookies to the database
    void save() override;

    /// Loads cookies from the database
    void load() override;

private:
    /// Used to access prepared database queries
    enum class StoredQuery
    {
        InsertCookie,
        DeleteCookie
    };

    /// Removes expired cookies from both the database and the list in memory
    void removeExpired();

private:
    /// True if private browsing cookie jar (e.g., no persistence), false if standard cookie jar
    bool m_privateJar;

    /// Map of commonly-used prepared database statements
    std::map< StoredQuery, std::unique_ptr<QSqlQuery> > m_queryMap;
};

#endif // COOKIEJAR_H
