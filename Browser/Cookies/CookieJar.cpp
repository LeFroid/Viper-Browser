#include <QDateTime>
#include <QNetworkCookie>
#include <QSqlError>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QDebug>

#include "CookieJar.h"

#include <QDebug>

CookieJar::CookieJar(const QString &databaseFile, QString name, bool privateJar, QObject *parent) :
    QNetworkCookieJar(parent),
    DatabaseWorker(databaseFile, name),
    m_privateJar(privateJar)
{
}

CookieJar::~CookieJar()
{
    if (!m_privateJar)
        save();
}

bool CookieJar::hasCookiesFor(const QString &host) const
{
    if (host.isEmpty())
        return false;

    // If host string of format "xxx.yyy.zzz", must check for any cookies ending
    // in ".yyy.zzz"
    QString hostSearch = host;
    int numDots = host.count(QChar('.'));
    if (numDots > 1)
        hostSearch = host.right(host.size() - host.indexOf(QChar('.'), 0));

    auto cookies = allCookies();
    for (const QNetworkCookie &cookie : cookies)
        if (cookie.domain().endsWith(hostSearch))
            return true;
    return false;
}

void CookieJar::clearCookiesFrom(const QDateTime &start)
{
    if (m_privateJar || !start.isValid() || start.isNull())
        return;

    QSqlQuery query(m_database);
    query.prepare("DELETE FROM Cookies WHERE DateCreated > (:date)");
    query.bindValue(":date", start.toMSecsSinceEpoch());
    if (!query.exec())
        qDebug() << "[Warning]: In CookieJar::clearCookiesFrom(..) - Could not erase cookies newer than "
                 << start.toString() << " from database";

    // reload cookies
    load();
}

void CookieJar::clearCookiesInRange(std::pair<QDateTime, QDateTime> range)
{
    if (m_privateJar)
        return;

    QSqlQuery query(m_database);
    query.prepare("DELETE FROM Cookies WHERE DateCreated > (:startDate) AND DateCreated < (:endDate)");
    query.bindValue(":startDate", range.first.toMSecsSinceEpoch());
    query.bindValue(":endDate", range.second.toMSecsSinceEpoch());
    if (!query.exec())
        qDebug() << "[Warning]: In CookieJar::clearCookiesInRaneg(..) - Could not erase cookies";

    // reload cookies
    load();
}

bool CookieJar::insertCookie(const QNetworkCookie &cookie)
{
    if (QNetworkCookieJar::insertCookie(cookie))
    {
        if (!cookie.isSessionCookie() && !m_privateJar)
        {
            QSqlQuery query(m_database);
            query.prepare("INSERT OR REPLACE INTO Cookies(Domain, Name, Path, EncryptedOnly, HttpOnly, "
                          "ExpirationDate, Value, DateCreated) VALUES(:domain, :name, :path, :encryptedOnly, "
                          ":httpOnly, :expireDate, :value, :dateCreated)");
            query.bindValue(":domain", cookie.domain());
            query.bindValue(":name", cookie.name());
            query.bindValue(":path", cookie.path());
            query.bindValue(":encryptedOnly", cookie.isSecure() ? 1 : 0);
            query.bindValue(":httpOnly", cookie.isHttpOnly() ? 1 : 0);
            query.bindValue(":expireDate", cookie.expirationDate().toString());
            query.bindValue(":value", cookie.value());
            query.bindValue(":dateCreated", QDateTime::currentMSecsSinceEpoch());
            if (!query.exec())
                qDebug() << "[Warning]: In CookieJar::insertCookie - Could not insert new cookie into database";
        }
        emit cookieAdded();
        return true;
    }
    return false;
}

bool CookieJar::deleteCookie(const QNetworkCookie &cookie)
{
    if (QNetworkCookieJar::deleteCookie(cookie))
    {
        if (m_privateJar)
            return true;

        QSqlQuery query(m_database);
        query.prepare("DELETE FROM Cookies WHERE Domain = (:domain) AND Name = (:name)");
        query.bindValue(":domain", cookie.domain());
        query.bindValue(":name", cookie.name());
        query.exec();
        return true;
    }
    return false;
}

void CookieJar::eraseAllCookies()
{
    QList<QNetworkCookie> noCookies;
    setAllCookies(noCookies);

    if (!m_privateJar)
    {
        if (!exec("DELETE FROM Cookies"))
            qDebug() << "[Warning]: In CookieJar::eraseAllCookies() - could not delete cookies from database.";
    }

    emit cookiesRemoved();
}

void CookieJar::setup()
{
    // Setup table structure
    QSqlQuery query(m_database);
    query.prepare("CREATE TABLE IF NOT EXISTS Cookies(Domain TEXT NOT NULL, Name TEXT NOT NULL, Path TEXT NOT NULL,"
                  "EncryptedOnly INTEGER NOT NULL DEFAULT 0, HttpOnly INTEGER NOT NULL DEFAULT 0,"
                  "ExpirationDate TEXT NOT NULL, Value BLOB, DateCreated INTEGER NOT NULL, PRIMARY KEY(Domain, Name))");
    if (!query.exec())
        qDebug() << "[Error]: In CookieJar::setup - unable to create cookie storage table in database. Message: " << query.lastError().text();
}

void CookieJar::save()
{
    removeExpired();
}

void CookieJar::load()
{
    if (m_privateJar)
        return;

    QList<QNetworkCookie> cookies;

    // Load cookies and remove any that have expired
    QSqlQuery query(m_database);
    query.prepare("SELECT * FROM Cookies");
    if (query.exec())
    {
        QSqlRecord rec = query.record();
        int idDomain = rec.indexOf("Domain");
        int idName = rec.indexOf("Name");
        int idPath = rec.indexOf("Path");
        int idEncrypt = rec.indexOf("EncryptedOnly");
        int idHttpOnly = rec.indexOf("HttpOnly");
        int idExpDate = rec.indexOf("ExpirationDate");
        int idValue = rec.indexOf("Value");
        while (query.next())
        {
            QNetworkCookie cookie(query.value(idName).toByteArray(), query.value(idValue).toByteArray());
            cookie.setDomain(query.value(idDomain).toString());
            cookie.setHttpOnly(query.value(idHttpOnly).toBool());
            cookie.setSecure(query.value(idEncrypt).toBool());
            cookie.setPath(query.value(idPath).toString());
            cookie.setExpirationDate(QDateTime::fromString(query.value(idExpDate).toString()));
            cookies.append(cookie);
        }
    }
    else
        qDebug() << "[Error]: In CookieJar::load - unable to fetch cookies from database. Message: " << query.lastError().text();

    setAllCookies(cookies);
    removeExpired();
}

void CookieJar::removeExpired()
{
    if (m_privateJar)
        return;

    QList<QNetworkCookie> cookies = allCookies();
    if (cookies.empty())
        return;

    // Prepare query and then iterate through the list
    QSqlQuery query(m_database);
    query.prepare("DELETE FROM Cookies WHERE ExpirationDate = (:expireDate)");

    QDateTime now = QDateTime::currentDateTime();
    int numCookies = cookies.size();
    for (int i = numCookies - 1; i >= 0; --i)
    {
        const QNetworkCookie &cookie = cookies.at(i);
        QDateTime expireDate = cookie.expirationDate();
        if (cookie.isSessionCookie() || expireDate > now)
            continue;

        query.bindValue(":expireDate", expireDate.toString());
        if (!query.exec())
            qDebug() << "[Error]: In CookieJar::removeExpired - unable to remove expired cookie from database. Message: " << query.lastError().text();

        cookies.removeAt(i);
    }

    setAllCookies(cookies);
}
