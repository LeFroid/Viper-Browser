#ifndef COOKIEJAR_H
#define COOKIEJAR_H

#include "DatabaseWorker.h"
#include <QDateTime>
#include <QNetworkCookieJar>

/**
 * @class CookieJar
 * @brief Implements ability to store and load cookies on disk
 */
class CookieJar : public QNetworkCookieJar, private DatabaseWorker
{
    friend class CookieTableModel;
    friend class DatabaseWorker;

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
    /// Sets initial table structure for storage of cookies
    void setup() override;

    /// Saves cookies to the database
    void save() override;

    /// Loads cookies from the database
    void load() override;

private:
    /// Removes expired cookies from both the database and the list in memory
    void removeExpired();

private:
    /// True if private browsing cookie jar (e.g., no persistence), false if standard cookie jar
    bool m_privateJar;
};

#endif // COOKIEJAR_H
