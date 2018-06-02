#ifndef COOKIEJAR_H
#define COOKIEJAR_H

#include "DatabaseWorker.h"
#include <map>
#include <memory>
#include <QDateTime>
#include <QNetworkCookieJar>
#include <QSqlQuery>
#include <QWebEngineCookieStore>

/**
 * @class CookieJar
 * @brief Implements ability to store and load cookies on disk
 */
class CookieJar : public QNetworkCookieJar
{
    friend class CookieTableModel;

    Q_OBJECT

public:
    /// Constructs the cookie jar
    explicit CookieJar(bool privateJar = false, QObject *parent =nullptr);

    /// Saves cookies to database before calling ~QNetworkCookieJar
    ~CookieJar();

    /// Returns true if the CookieJar contains 1 or more cookies associated with the given host, false if else
    bool hasCookiesFor(const QString &host) const;

    /// Erases all cookies from the jar
    void eraseAllCookies();

signals:
    /// Emitted when a new cookie has been added to the jar
    void cookieAdded();

    /// Emitted when all the cookies have been erased
    void cookiesRemoved();

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

    /// Pointer to the web engine's cookie store
    QWebEngineCookieStore *m_store;
};

#endif // COOKIEJAR_H
