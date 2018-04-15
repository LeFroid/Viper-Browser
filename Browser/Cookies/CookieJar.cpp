#include <QDateTime>
#include <QNetworkCookie>
#include <QSqlError>
#include <QSqlRecord>
#include <QWebEngineProfile>
#include <QDebug>

#include "CookieJar.h"


CookieJar::CookieJar(const QString &databaseFile, QString name, bool privateJar, QObject *parent) :
    QNetworkCookieJar(parent),
    DatabaseWorker(databaseFile, name),
    m_privateJar(privateJar),
    m_queryMap(),
    m_store(nullptr)
{
    // If not in private mode, load cookies from storage
    if (!m_privateJar)
    {
        m_store = QWebEngineProfile::defaultProfile()->cookieStore();
        connect(m_store, &QWebEngineCookieStore::cookieAdded, [=](const QNetworkCookie &cookie){
            static_cast<void>(insertCookie(cookie));
        });
        connect(m_store, &QWebEngineCookieStore::cookieRemoved, [=](const QNetworkCookie &cookie){
            static_cast<void>(deleteCookie(cookie));
        });
    }
    // Prepare commonly used queries and store in the query map
    auto savedQuery = std::make_unique<QSqlQuery>(m_database);
    savedQuery->prepare(QLatin1String("INSERT OR REPLACE INTO Cookies(Domain, Name, Path, EncryptedOnly, HttpOnly, "
                                      "ExpirationDate, Value, DateCreated) VALUES(:domain, :name, :path, :encryptedOnly, "
                                      ":httpOnly, :expireDate, :value, :dateCreated)"));
    m_queryMap[StoredQuery::InsertCookie] = std::move(savedQuery);

    savedQuery = std::make_unique<QSqlQuery>(m_database);
    savedQuery->prepare(QLatin1String("DELETE FROM Cookies WHERE Domain = (:domain) AND Name = (:name)"));
    m_queryMap[StoredQuery::DeleteCookie] = std::move(savedQuery);
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
    query.prepare(QLatin1String("DELETE FROM Cookies WHERE DateCreated > (:date)"));
    query.bindValue(QLatin1String(":date"), start.toMSecsSinceEpoch());
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
    query.prepare(QLatin1String("DELETE FROM Cookies WHERE DateCreated > (:startDate) AND DateCreated < (:endDate)"));
    query.bindValue(QLatin1String(":startDate"), range.first.toMSecsSinceEpoch());
    query.bindValue(QLatin1String(":endDate"), range.second.toMSecsSinceEpoch());
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
            QSqlQuery *query = m_queryMap.at(StoredQuery::InsertCookie).get();
            query->bindValue(QLatin1String(":domain"), cookie.domain());
            query->bindValue(QLatin1String(":name"), cookie.name());
            query->bindValue(QLatin1String(":path"), cookie.path());
            query->bindValue(QLatin1String(":encryptedOnly"), cookie.isSecure() ? 1 : 0);
            query->bindValue(QLatin1String(":httpOnly"), cookie.isHttpOnly() ? 1 : 0);
            query->bindValue(QLatin1String(":expireDate"), cookie.expirationDate().toString());
            query->bindValue(QLatin1String(":value"), cookie.value());
            query->bindValue(QLatin1String(":dateCreated"), QDateTime::currentMSecsSinceEpoch());
            if (!query->exec())
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
        else
            m_store->deleteCookie(cookie);

        QSqlQuery *query = m_queryMap.at(StoredQuery::DeleteCookie).get();
        query->bindValue(QLatin1String(":domain"), cookie.domain());
        query->bindValue(QLatin1String(":name"), cookie.name());
        return query->exec();
    }
    return false;
}

void CookieJar::eraseAllCookies()
{
    QList<QNetworkCookie> noCookies;
    setAllCookies(noCookies);

    if (!m_privateJar)
    {
        m_store->deleteAllCookies();
        if (!exec("DELETE FROM Cookies"))
            qDebug() << "[Warning]: In CookieJar::eraseAllCookies() - could not delete cookies from database.";
    }

    emit cookiesRemoved();
}

bool CookieJar::hasProperStructure()
{
    // Verify existence of Cookies table
    return hasTable(QStringLiteral("Cookies"));
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

    m_store->deleteAllCookies();

    QList<QNetworkCookie> cookies;

    // Load cookies and remove any that have expired
    QSqlQuery query(m_database);
    query.prepare(QLatin1String("SELECT * FROM Cookies"));
    if (query.exec())
    {
        QSqlRecord rec = query.record();
        int idDomain = rec.indexOf(QLatin1String("Domain"));
        int idName = rec.indexOf(QLatin1String("Name"));
        int idPath = rec.indexOf(QLatin1String("Path"));
        int idEncrypt = rec.indexOf(QLatin1String("EncryptedOnly"));
        int idHttpOnly = rec.indexOf(QLatin1String("HttpOnly"));
        int idExpDate = rec.indexOf(QLatin1String("ExpirationDate"));
        int idValue = rec.indexOf(QLatin1String("Value"));
        while (query.next())
        {
            QNetworkCookie cookie(query.value(idName).toByteArray(), query.value(idValue).toByteArray());
            cookie.setDomain(query.value(idDomain).toString());
            cookie.setHttpOnly(query.value(idHttpOnly).toBool());
            cookie.setSecure(query.value(idEncrypt).toBool());
            cookie.setPath(query.value(idPath).toString());
            cookie.setExpirationDate(QDateTime::fromString(query.value(idExpDate).toString()));
            cookies.append(cookie);

            m_store->setCookie(cookie);
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
    query.prepare(QLatin1String("DELETE FROM Cookies WHERE ExpirationDate = (:expireDate)"));

    QDateTime now = QDateTime::currentDateTime();
    int numCookies = cookies.size();
    for (int i = numCookies - 1; i >= 0; --i)
    {
        const QNetworkCookie &cookie = cookies.at(i);
        QDateTime expireDate = cookie.expirationDate();
        if (cookie.isSessionCookie() || expireDate > now)
            continue;

        query.bindValue(QLatin1String(":expireDate"), expireDate.toString());
        if (!query.exec())
            qDebug() << "[Error]: In CookieJar::removeExpired - unable to remove expired cookie from database. Message: " << query.lastError().text();

        m_store->deleteCookie(cookie);
        cookies.removeAt(i);
    }

    setAllCookies(cookies);
}
