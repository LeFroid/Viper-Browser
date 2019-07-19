#include "BrowserApplication.h"
#include "CommonUtil.h"
#include "HistoryStore.h"
#include "Settings.h"
#include "WebWidget.h"

#include <array>
#include <QBuffer>
#include <QCryptographicHash>
#include <QDateTime>
#include <QFuture>
#include <QIcon>
#include <QImage>
#include <QRegularExpression>
#include <QSqlError>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QtConcurrent>
#include <QUrl>
#include <QDebug>

HistoryStore::HistoryStore(const QString &databaseFile) :
    DatabaseWorker(databaseFile, QLatin1String("HistoryDB")),
    m_lastVisitID(0),
    m_entries(),
    m_recentItems(),
    m_queryUpdateHistoryItem(nullptr),
    m_queryHistoryItem(nullptr),
    m_queryVisit(nullptr),
    m_queryGetEntryByUrl(nullptr)
{
}

HistoryStore::~HistoryStore()
{
    if (m_queryGetEntryByUrl)
    {
        delete m_queryGetEntryByUrl;
        m_queryGetEntryByUrl = nullptr;
    }

    if (m_queryUpdateHistoryItem != nullptr)
    {
        delete m_queryUpdateHistoryItem;
        m_queryUpdateHistoryItem = nullptr;

        delete m_queryHistoryItem;
        m_queryHistoryItem = nullptr;

        delete m_queryVisit;
        m_queryVisit = nullptr;
    }
}

void HistoryStore::clearAllHistory()
{
    if (!exec(QLatin1String("DELETE FROM History")))
        qWarning() << "In HistoryStore::clearAllHistory - Unable to clear History table.";

    if (!exec(QLatin1String("DELETE FROM Visits")))
        qWarning() << "In HistoryStore::clearAllHistory - Unable to clear Visits table.";
}

void HistoryStore::clearHistoryFrom(const QDateTime &start)
{
    QSqlQuery query(m_database);
    query.prepare(QLatin1String("DELETE FROM Visits WHERE Date >= (:date)"));
    query.bindValue(QLatin1String(":date"), start.toMSecsSinceEpoch());
    if (!query.exec())
        qWarning() << "In HistoryStore::clearHistoryFrom - Unable to clear history. Message: "
                   << query.lastError().text();

    if (!query.exec(QLatin1String("DELETE FROM History WHERE VisitID NOT IN (SELECT DISTINCT VisitID FROM Visits)")))
        qWarning() << "In HistoryStore::clearHistoryFrom - Unable to clear history. Message: "
                   << query.lastError().text();
}

void HistoryStore::clearHistoryInRange(std::pair<QDateTime, QDateTime> range)
{
    QSqlQuery query(m_database);
    query.prepare(QLatin1String("DELETE FROM Visits WHERE Date >= (:startDate) AND Date <= (:endDate)"));
    query.bindValue(QLatin1String(":startDate"), range.first.toMSecsSinceEpoch());
    query.bindValue(QLatin1String(":endDate"), range.second.toMSecsSinceEpoch());
    if (!query.exec())
        qWarning() << "In HistoryStore::clearHistoryFrom - Unable to clear history. Message: "
                   << query.lastError().text();

    if (!query.exec(QLatin1String("DELETE FROM History WHERE VisitID NOT IN (SELECT DISTINCT VisitID FROM Visits)")))
        qWarning() << "In HistoryStore::clearHistoryFrom - Unable to clear history. Message: "
                   << query.lastError().text();
}

bool HistoryStore::contains(const QUrl &url) const
{
    QSqlQuery query(m_database);
    query.prepare(QLatin1String("SELECT VisitID FROM History WHERE URL = (:url)"));
    query.bindValue(QLatin1String(":url"), url);
    return query.exec() && query.next();
}

HistoryEntry HistoryStore::getEntry(const QUrl &url)
{
    HistoryEntry result;
    result.URL = url;
    result.VisitID = -1;
    if (!m_queryGetEntryByUrl)
    {
        m_queryGetEntryByUrl = new QSqlQuery(m_database);
        m_queryGetEntryByUrl->prepare(QLatin1String("SELECT Visits.VisitID, Visits.Date, MostRecentVisit.NumVisits,"
                              " History.Title, History.URLTypedCount FROM "
                              " (SELECT VisitID, MAX(Date) AS Date, COUNT(VisitID) AS NumVisits "
                              " FROM Visits "
                              " GROUP BY VisitID) AS MostRecentVisit "
                              " INNER JOIN Visits ON "
                              "  Visits.VisitID = MostRecentVisit.VisitID AND "
                              "  Visits.Date = MostRecentVisit.Date "
                              " INNER JOIN History "
                              "  ON History.VisitID = Visits.VisitID "
                              " WHERE History.URL = (:url)"));
    }

    m_queryGetEntryByUrl->bindValue(QLatin1String(":url"), url);
    if (!m_queryGetEntryByUrl->exec())
        qWarning() << "In HistoryStore::getEntry - Unable to execute search query. Message: "
                   << m_queryGetEntryByUrl->lastError().text();

    if (m_queryGetEntryByUrl->first())
    {
        result.VisitID   = m_queryGetEntryByUrl->value(0).toInt();
        result.LastVisit = QDateTime::fromMSecsSinceEpoch(m_queryGetEntryByUrl->value(1).toULongLong());
        result.NumVisits = m_queryGetEntryByUrl->value(2).toInt();
        result.Title     = m_queryGetEntryByUrl->value(3).toString();
        result.URLTypedCount = m_queryGetEntryByUrl->value(4).toInt();
    }

    return result;
}

std::vector<URLRecord> HistoryStore::getEntries() const
{
    return m_entries;
}

std::vector<VisitEntry> HistoryStore::getVisits(const HistoryEntry &record)
{
    std::vector<VisitEntry> result;

    QSqlQuery query(m_database);
    query.prepare(QLatin1String("SELECT Date FROM Visits WHERE VisitID = (:visitId) ORDER BY Date ASC"));
    query.bindValue(QLatin1String(":visitId"), record.VisitID);
    if (query.exec())
    {
        while (query.next())
        {
            result.push_back(QDateTime::fromMSecsSinceEpoch(query.value(0).toLongLong()));
        }
    }

    return result;
}

std::deque<HistoryEntry> HistoryStore::getRecentItems()
{
    std::deque<HistoryEntry> result;

    QSqlQuery query(m_database);
    if (!query.exec(QLatin1String("SELECT Visits.VisitID, Visits.Date, History.URL, History.Title, "
                                  "History.URLTypedCount FROM Visits INNER JOIN History ON "
                                  "Visits.VisitID = History.VisitID "
                                  "ORDER BY Visits.Date DESC LIMIT 15")))
    {
        qWarning() << "HistoryStore - could not load recently visited history entries";
        return result;
    }

    while (query.next())
    {
        HistoryEntry entry;
        entry.VisitID   = query.value(0).toInt();
        entry.LastVisit = QDateTime::fromMSecsSinceEpoch(query.value(1).toULongLong());
        entry.URL       = query.value(2).toUrl();
        entry.Title     = query.value(3).toString();
        entry.URLTypedCount = query.value(4).toInt();
        entry.NumVisits = 1;

        result.push_back(std::move(entry));
    }

    return result;
}

std::vector<URLRecord> HistoryStore::getHistoryFrom(const QDateTime &startDate) const
{
    return getHistoryBetween(startDate, QDateTime::currentDateTime());
}

std::vector<URLRecord> HistoryStore::getHistoryBetween(const QDateTime &startDate, const QDateTime &endDate) const
{
    std::vector<URLRecord> result;

    if (!startDate.isValid() || !endDate.isValid())
        return result;

    // Get startDate in msec format
    qint64 startMSec = startDate.toMSecsSinceEpoch();
    qint64 endMSec = endDate.toMSecsSinceEpoch();

    // Setup queries
    QSqlQuery queryVisitIds(m_database), queryHistoryItem(m_database), queryVisitDates(m_database);
    queryVisitIds.prepare(QLatin1String("SELECT DISTINCT VisitID FROM Visits WHERE Date >= (:startDate) AND Date <= (:endDate) "
                                        " ORDER BY Date ASC"));
    queryHistoryItem.prepare(QLatin1String("SELECT URL, Title, URLTypedCount FROM History WHERE VisitID = (:visitId)"));
    queryVisitDates.prepare(QLatin1String("SELECT Date FROM Visits WHERE VisitID = (:visitId) AND Date >= "
                                          "(:startDate) AND Date <= (:endDate) ORDER BY Date ASC"));

    queryVisitIds.bindValue(QLatin1String(":startDate"), startMSec);
    queryVisitIds.bindValue(QLatin1String(":endDate"), endMSec);
    if (!queryVisitIds.exec())
        qDebug() << "HistoryStore::getHistoryBetween - error executing query. Message: " << queryVisitIds.lastError().text();

    while (queryVisitIds.next())
    {
        const int visitId = queryVisitIds.value(0).toInt();

        queryHistoryItem.bindValue(QLatin1String(":visitId"), visitId);
        if (!queryHistoryItem.exec() || !queryHistoryItem.next())
            continue;

        HistoryEntry entry;
        entry.URL = queryHistoryItem.value(0).toUrl();
        entry.Title = queryHistoryItem.value(1).toString();
        entry.URLTypedCount = queryHistoryItem.value(2).toInt();
        entry.VisitID = visitId;

        std::vector<VisitEntry> visits;
        queryVisitDates.bindValue(QLatin1String(":visitId"), visitId);
        queryVisitDates.bindValue(QLatin1String(":startDate"), startMSec);
        queryVisitDates.bindValue(QLatin1String(":endDate"), endMSec);
        if (queryVisitDates.exec())
        {
            while (queryVisitDates.next())
            {
                VisitEntry visit = QDateTime::fromMSecsSinceEpoch(queryVisitDates.value(0).toLongLong());
                visits.push_back(visit);
            }
        }

        entry.LastVisit = visits.at(visits.size() - 1);
        entry.NumVisits = static_cast<int>(visits.size());
        result.push_back( URLRecord{ std::move(entry), std::move(visits) } );
    }

    return result;
}

int HistoryStore::getTimesVisitedHost(const QUrl &url) const
{
    QSqlQuery query(m_database);
    query.prepare(QLatin1String("SELECT COUNT(NumVisits) FROM (SELECT VisitID, COUNT(VisitID) AS NumVisits "
                                "FROM Visits GROUP BY VisitID ) WHERE VisitID IN (SELECT VisitID FROM History "
                                "WHERE URL LIKE (:url))"));
    query.bindValue(QLatin1String(":url"), QString("%%1%").arg(url.host().remove(QRegularExpression("^www\\.")).toLower()));
    if (query.exec() && query.first())
        return query.value(0).toInt();
    return 0;
}

int HistoryStore::getTimesVisited(const QUrl &url) const
{
    QSqlQuery query(m_database);
    query.prepare(QLatin1String("SELECT h.VisitID, v.NumVisits FROM History AS h "
                                "INNER JOIN (SELECT VisitID, COUNT(VisitID) AS NumVisits "
                                "FROM Visits GROUP BY VisitID) AS v "
                                "ON h.VisitID = v.VisitID WHERE h.URL = (:url)"));
    query.bindValue(QLatin1String(":url"), url);
    if (query.exec() && query.first())
    {
        return query.value(1).toInt();
    }

    return 0;
}

void HistoryStore::addVisit(const QUrl &url, const QString &title, const QDateTime &visitTime, const QUrl &requestedUrl, bool wasTypedByUser)
{
    if (m_queryUpdateHistoryItem == nullptr)
    {
        m_queryUpdateHistoryItem = new QSqlQuery(m_database);
        m_queryUpdateHistoryItem->prepare(
                    QLatin1String("UPDATE History SET Title = (:title), URLTypedCount = (:urlTypedCount) WHERE VisitID = (:visitId)"));

        m_queryHistoryItem = new QSqlQuery(m_database);
        m_queryHistoryItem->prepare(
                    QLatin1String("INSERT OR REPLACE INTO History(VisitID, URL, Title, URLTypedCount) "
                                  "VALUES(:visitId, :url, :title, :urlTypedCount)"));

        m_queryVisit = new QSqlQuery(m_database);
        m_queryVisit->prepare(
                    QLatin1String("INSERT INTO Visits(VisitID, Date) VALUES(:visitId, :date)"));
    }

    auto existingEntry = getEntry(url);
    qulonglong visitId = existingEntry.VisitID >= 0 ? existingEntry.VisitID : ++m_lastVisitID;
    if (existingEntry.VisitID >= 0)
    {
        if (wasTypedByUser)
            existingEntry.URLTypedCount++;

        m_queryUpdateHistoryItem->bindValue(QLatin1String(":title"), title);
        m_queryUpdateHistoryItem->bindValue(QLatin1String(":visitId"), existingEntry.VisitID);
        m_queryUpdateHistoryItem->bindValue(QLatin1String(":urlTypedCount"), existingEntry.URLTypedCount);

        if (!m_queryUpdateHistoryItem->exec())
            qWarning() << "HistoryStore::addVisit - could not save entry to database. Error: "
                       << m_queryUpdateHistoryItem->lastError().text();
    }
    else
    {
        const int urlTypedCount = wasTypedByUser ? 1 : 0;

        m_queryHistoryItem->bindValue(QLatin1String(":visitId"), visitId);
        m_queryHistoryItem->bindValue(QLatin1String(":url"), url);
        m_queryHistoryItem->bindValue(QLatin1String(":title"), title);
        m_queryHistoryItem->bindValue(QLatin1String(":urlTypedCount"), urlTypedCount);
        if (!m_queryHistoryItem->exec())
            qWarning() << "HistoryStore::addVisit - could not save entry to database. Error: "
                       << m_queryHistoryItem->lastError().text();
    }

    const quint64 visitTimeInMSec = static_cast<quint64>(visitTime.toMSecsSinceEpoch());
    m_queryVisit->bindValue(QLatin1String(":visitId"), visitId);
    m_queryVisit->bindValue(QLatin1String(":date"), visitTimeInMSec);
    if (!m_queryVisit->exec())
        qWarning() << "HistoryStore::addVisit - could not save visit to database. Error: "
                   << m_queryVisit->lastError().text();

    if (!CommonUtil::doUrlsMatch(url, requestedUrl, true))
    {
        QDateTime requestDateTime = visitTime.addMSecs(-100);
        if (!requestDateTime.isValid())
            requestDateTime = visitTime;
        addVisit(requestedUrl, title, requestDateTime, requestedUrl, wasTypedByUser);
    }
}

bool HistoryStore::hasProperStructure()
{
    // Verify existence of Visits and History tables
    return hasTable(QLatin1String("Visits")) && hasTable(QLatin1String("History"));
}

void HistoryStore::setup()
{
    QSqlQuery query(m_database);
    if (!query.exec(QLatin1String("CREATE TABLE History(VisitID INTEGER PRIMARY KEY AUTOINCREMENT, URL TEXT UNIQUE NOT NULL, Title TEXT, "
                                  "URLTypedCount INTEGER DEFAULT 0)")))
    {
        qDebug() << "[Error]: In HistoryStore::setup - unable to create history table. Message: " << query.lastError().text();
    }
    if (!query.exec(QLatin1String("CREATE TABLE Visits(VisitID INTEGER NOT NULL, Date INTEGER NOT NULL, "
                                  "FOREIGN KEY(VisitID) REFERENCES History(VisitID) ON DELETE CASCADE, PRIMARY KEY(VisitID, Date))")))
    {
        qDebug() << "[Error]: In HistoryStore::setup - unable to create visited table. Message: " << query.lastError().text();
    }
}

void HistoryStore::load()
{
    m_entries.clear();
    purgeOldEntries();
    checkForUpdate();

    // Load current state of history entries from the DB
    QSqlQuery query(m_database);
    QSqlQuery queryRecentVisit(m_database);
    //QSqlQuery queryAllVisits(m_database);

    queryRecentVisit.prepare(
                QLatin1String("SELECT MAX(Date) AS Date, COUNT(VisitID) AS NumVisits "
                              "FROM Visits WHERE VisitID = (:visitId)"));

    //queryAllVisits.prepare(QLatin1String("SELECT Date FROM Visits WHERE VisitID = (:visitId) ORDER BY Date ASC"));

    // Clear history entries that are not referenced by any specific visits
    if (!query.exec(QLatin1String("DELETE FROM History WHERE VisitID NOT IN (SELECT DISTINCT VisitID FROM Visits)")))
    {
        qWarning() << "In HistoryStore::load - Could not remove non-referenced history entries from the database."
                   << " Message: " << query.lastError().text();
    }

    // Now load history entries
    if (query.exec(QLatin1String("SELECT VisitID, URL, Title, URLTypedCount FROM History ORDER BY VisitID ASC")))
    {
        QSqlRecord rec = query.record();
        const int idVisit = rec.indexOf(QLatin1String("VisitID"));
        const int idUrl = rec.indexOf(QLatin1String("URL"));
        const int idTitle = rec.indexOf(QLatin1String("Title"));
        const int idUrlTypedCount = rec.indexOf(QLatin1String("URLTypedCount"));
        while (query.next())
        {
            HistoryEntry entry;
            entry.URL     = query.value(idUrl).toUrl();
            entry.Title   = query.value(idTitle).toString();
            entry.VisitID = query.value(idVisit).toInt();
            entry.URLTypedCount = query.value(idUrlTypedCount).toInt();

            queryRecentVisit.bindValue(QLatin1String(":visitId"), entry.VisitID);
            if (queryRecentVisit.exec() && queryRecentVisit.first())
            {
                entry.LastVisit = QDateTime::fromMSecsSinceEpoch(queryRecentVisit.value(0).toULongLong());
                entry.NumVisits = queryRecentVisit.value(1).toInt();
            }

            m_entries.push_back(URLRecord{ std::move(entry) });
        }
    }
    else
        qWarning() << "In HistoryStore::load - Unable to fetch history items from the database. Message: "
                   << query.lastError().text();

    if (query.exec(QLatin1String("SELECT MAX(VisitID) FROM History")) && query.first())
        m_lastVisitID = query.value(0).toULongLong();
}

void HistoryStore::save()
{
}

void HistoryStore::checkForUpdate()
{
    // Check if table structure needs update before loading
    QSqlQuery query(m_database);
    if (query.exec(QLatin1String("PRAGMA table_info(History)")))
    {
        bool hasUrlTypeCountColumn = false;
        const QString urlTypeCountColumn("URLTypedCount");

        while (query.next())
        {
            QString columnName = query.value(1).toString();
            if (columnName.compare(urlTypeCountColumn) == 0)
                hasUrlTypeCountColumn = true;
        }

        if (!hasUrlTypeCountColumn)
        {
            if (!query.exec(QLatin1String("ALTER TABLE History ADD URLTypedCount INTEGER DEFAULT 0")))
                qDebug() << "Error updating history table with url typed count column";
        }
    }
}

void HistoryStore::purgeOldEntries()
{
    // Clear visits that are 6+ months old
    QSqlQuery query(m_database);
    quint64 purgeDate = static_cast<quint64>(QDateTime::currentMSecsSinceEpoch());
    const quint64 tmp = quint64{15552000000};
    if (purgeDate > tmp)
    {
        purgeDate -= tmp;
        query.prepare(QLatin1String("DELETE FROM Visits WHERE Date < (:purgeDate)"));
        query.bindValue(QLatin1String(":purgeDate"), purgeDate);
        if (!query.exec())
        {
            qWarning() << "HistoryStore - Could not purge old history entries. Message: " << query.lastError().text();
        }
    }
}

std::vector<WebPageInformation> HistoryStore::loadMostVisitedEntries(int limit)
{
    std::vector<WebPageInformation> result;
    if (limit <= 0)
        return result;

    QSqlQuery query(m_database);
    query.prepare(QLatin1String("SELECT v.VisitID, COUNT(v.VisitID) AS NumVisits, h.URL, h.Title FROM "
                                "Visits AS v JOIN History AS h ON v.VisitID = h.VisitID GROUP BY v.VisitID "
                                "ORDER BY NumVisits DESC LIMIT (:resultLimit)"));
    query.bindValue(QLatin1String(":resultLimit"), limit);
    if (!query.exec())
    {
        qWarning() << "In HistoryStore::loadMostVisitedEntries - unable to load most frequently visited entries. Message: " << query.lastError().text();
        return result;
    }

    QSqlRecord rec = query.record();
    int idUrl = rec.indexOf(QLatin1String("URL"));
    int idTitle = rec.indexOf(QLatin1String("Title"));
    int count = 0;
    while (query.next())
    {
        WebPageInformation item;
        item.Position = count++;
        item.URL = query.value(idUrl).toUrl();
        item.Title = query.value(idTitle).toString();
        result.push_back(item);
    }

    return result;
}

