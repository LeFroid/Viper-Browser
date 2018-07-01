#include "BrowserApplication.h"
#include "HistoryManager.h"
#include "Settings.h"

#include <QBuffer>
#include <QDateTime>
#include <QIcon>
#include <QImage>
#include <QRegExp>
#include <QSqlError>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QUrl>
#include <QDebug>

HistoryManager::HistoryManager(const QString &databaseFile, QObject *parent) :
    QObject(parent),
    DatabaseWorker(databaseFile, QLatin1String("HistoryDB")),
    m_lastVisitID(0),
    m_historyItems(),
    m_recentItems(),
    m_queryHistoryItem(nullptr),
    m_queryVisit(nullptr),
    m_storagePolicy(HistoryStoragePolicy::Remember)
{
    m_storagePolicy = static_cast<HistoryStoragePolicy>(sBrowserApplication->getSettings()->getValue(BrowserSetting::HistoryStoragePolicy).toInt());
}

HistoryManager::~HistoryManager()
{
    switch (m_storagePolicy)
    {
        case HistoryStoragePolicy::Remember:
            break;
        case HistoryStoragePolicy::SessionOnly:
        case HistoryStoragePolicy::Never:
        default:
            clearAllHistory();
            break;
    }

    if (m_queryHistoryItem != nullptr)
    {
        delete m_queryHistoryItem;
        delete m_queryVisit;
    }
}

void HistoryManager::addHistoryEntry(const QString &url, const QString &title)
{
    // Check if qrc:/ resource, and if so, ignore
    if (url.startsWith(QLatin1String("qrc:")) || m_storagePolicy == HistoryStoragePolicy::Never)
        return;

    QDateTime visitTime = QDateTime::currentDateTime();

    QString urlFormatted = url;
    int hashPos = url.indexOf(QLatin1Char('#'));
    if (hashPos > 0)
        urlFormatted = urlFormatted.left(hashPos);

    const bool emptyTitle = title.isEmpty();

    auto it = m_historyItems.find(urlFormatted);
    if (it != m_historyItems.end())
    {
        it->Visits.prepend(visitTime);
        m_recentItems.push_front(*it);
        while (m_recentItems.size() > 15)
            m_recentItems.pop_back();
        if (it->Title.isEmpty() && !emptyTitle)
            it->Title = title;

        if (!it->Title.isEmpty())
        {
            saveVisit(*it, visitTime);
            emit pageVisited(urlFormatted, it->Title);
        }
        return;
    }

    WebHistoryItem item;
    item.URL = urlFormatted;
    item.VisitID = ++m_lastVisitID;
    if (!emptyTitle)
        item.Title = title;
    item.Visits.prepend(visitTime);

    m_historyItems.insert(urlFormatted, item);
    m_recentItems.push_front(item);
    while (m_recentItems.size() > 15)
        m_recentItems.pop_back();

    saveVisit(item, visitTime);
    emit pageVisited(urlFormatted, title);
}

void HistoryManager::clearAllHistory()
{
    if (!exec(QLatin1String("DELETE FROM History")))
        qDebug() << "[Error]: In HistoryManager::clearAllHistory - Unable to clear History table.";

    if (!exec(QLatin1String("DELETE FROM Visits")))
        qDebug() << "[Error]: In HistoryManager::clearAllHistory - Unable to clear Visits table.";

    m_recentItems.clear();
    m_historyItems.clear();
}

void HistoryManager::clearHistoryFrom(const QDateTime &start)
{
    // Perform database query and reload data
    QSqlQuery query(m_database);
    query.prepare(QLatin1String("DELETE FROM Visits WHERE Date > (:date)"));
    query.bindValue(QLatin1String(":date"), start.toMSecsSinceEpoch());
    if (!query.exec())
    {
        qDebug() << "[Error]: In HistoryManager::clearHistoryFrom - Unable to clear history. Message: "
                 << query.lastError().text();
        return;
    }

    m_recentItems.clear();
    m_historyItems.clear();
    load();
}

void HistoryManager::clearHistoryInRange(std::pair<QDateTime, QDateTime> range)
{
    // Perform database query and reload data
    QSqlQuery query(m_database);
    query.prepare(QLatin1String("DELETE FROM Visits WHERE Date > (:startDate) AND Date < (:endDate)"));
    query.bindValue(QLatin1String(":startDate"), range.first.toMSecsSinceEpoch());
    query.bindValue(QLatin1String(":endDate"), range.second.toMSecsSinceEpoch());
    if (!query.exec())
    {
        qDebug() << "[Error]: In HistoryManager::clearHistoryFrom - Unable to clear history. Message: "
                 << query.lastError().text();
        return;
    }

    m_recentItems.clear();
    m_historyItems.clear();
    load();
}

bool HistoryManager::historyContains(const QString &url) const
{
    return (m_historyItems.find(url) != m_historyItems.end());
}

std::vector<WebHistoryItem> HistoryManager::getHistoryFrom(const QDateTime &startDate) const
{
    /*std::vector<WebHistoryItem> items;

    if (!startDate.isValid())
        return items;

    // Get startDate in msec format
    qint64 startMSec = startDate.toMSecsSinceEpoch();

    // Prepare DB query (date is in MSecsSinceEpoch())
    QSqlQuery query(m_database);
    query.prepare(QLatin1String("SELECT Date FROM Visits WHERE VisitID = (:id) AND Date >= (:date)"));

    // Iterate through list of all visited URLs, searching the DB to confirm the date is between startDate and now
    for (const auto &it : m_historyItems)
    {
        WebHistoryItem item;
        item.Title = it.Title;
        item.URL = it.URL;
        item.VisitID = it.VisitID;
        //item.Visits = it.Visits;

        // Check DB
        query.bindValue(QLatin1String(":id"), it.VisitID);
        query.bindValue(QLatin1String(":date"), startMSec);
        if (query.exec())
        {
            while (query.next())
                item.Visits.append(QDateTime::fromMSecsSinceEpoch(query.value(0).toLongLong()));
        }
        items.push_back(item);
    }
    return items;*/
    return getHistoryBetween(startDate, QDateTime::currentDateTime());
}

std::vector<WebHistoryItem> HistoryManager::getHistoryBetween(const QDateTime &startDate, const QDateTime &endDate) const
{
    std::vector<WebHistoryItem> items;

    if (!startDate.isValid() || !endDate.isValid())
        return items;

    // Get startDate in msec format
    qint64 startMSec = startDate.toMSecsSinceEpoch();
    qint64 endMSec = endDate.toMSecsSinceEpoch();

    // Setup queries
    QSqlQuery queryVisitIds(m_database), queryHistoryItem(m_database), queryVisitDates(m_database);
    queryVisitIds.prepare(QLatin1String("SELECT DISTINCT VisitID FROM Visits WHERE Date > (:startDate) AND Date <= (:endDate)"));
    queryHistoryItem.prepare(QLatin1String("SELECT URL, Title FROM History WHERE VisitID = (:visitId)"));
    queryVisitDates.prepare(QLatin1String("SELECT Date FROM Visits WHERE VisitID = (:visitId) AND Date > (:startDate) AND Date <= (:endDate)"));

    queryVisitIds.bindValue(QLatin1String(":startDate"), startMSec);
    queryVisitIds.bindValue(QLatin1String(":endDate"), endMSec);
    if (!queryVisitIds.exec())
        qDebug() << "HistoryManager::getHistoryBetween - error executing query. Message: " << queryVisitIds.lastError().text();
    while (queryVisitIds.next())
    {
        const int visitId = queryVisitIds.value(0).toInt();

        queryHistoryItem.bindValue(QLatin1String(":visitId"), visitId);
        if (!queryHistoryItem.exec() || !queryHistoryItem.next())
            continue;

        WebHistoryItem item;
        item.URL = queryHistoryItem.value(0).toString();
        item.Title = queryHistoryItem.value(1).toString();
        item.VisitID = visitId;

        queryVisitDates.bindValue(QLatin1String(":visitId"), visitId);
        queryVisitDates.bindValue(QLatin1String(":startDate"), startMSec);
        queryVisitDates.bindValue(QLatin1String(":endDate"), endMSec);
        if (queryVisitDates.exec())
        {
            while (queryVisitDates.next())
                item.Visits.append(QDateTime::fromMSecsSinceEpoch(queryVisitDates.value(0).toLongLong()));
        }
        items.push_back(item);
    }

    return items;
}

int HistoryManager::getTimesVisited(const QString &host)
{
    QSqlQuery query(m_database);
    query.prepare(QLatin1String("SELECT COUNT(VisitID) FROM Visits WHERE VisitID = (:id)"));
    int timesVisited = 0;
    for (const WebHistoryItem &item : m_historyItems)
    {
        if (host.endsWith(item.URL.host().remove(QRegExp("(http(s)?://)?(www.)"))))
        {
            query.bindValue(QLatin1String(":id"), item.VisitID);
            if (query.exec() && query.next())
                timesVisited += query.value(0).toInt();
        }
    }
    return timesVisited;
}

HistoryStoragePolicy HistoryManager::getStoragePolicy() const
{
    return m_storagePolicy;
}

void HistoryManager::setStoragePolicy(HistoryStoragePolicy policy)
{
    m_storagePolicy = policy;

    if (policy == HistoryStoragePolicy::Never)
    {
        QDateTime invalidDate;
        sBrowserApplication->clearHistory(HistoryType::Browsing, invalidDate);
    }
}

bool HistoryManager::hasProperStructure()
{
    // Verify existence of Visits and History tables
    return hasTable(QLatin1String("Visits")) && hasTable(QLatin1String("History"));
}

void HistoryManager::setup()
{
    QSqlQuery query(m_database);
    if (!query.exec(QLatin1String("CREATE TABLE History(VisitID INTEGER PRIMARY KEY, URL TEXT UNIQUE NOT NULL, Title TEXT)")))
    {
        qDebug() << "[Error]: In HistoryManager::setup - unable to create history table. Message: " << query.lastError().text();
    }
    if (!query.exec(QLatin1String("CREATE TABLE Visits(VisitID INTEGER NOT NULL, Date INTEGER NOT NULL, "
                                  "FOREIGN KEY(VisitID) REFERENCES History(VisitID) ON DELETE CASCADE, PRIMARY KEY(VisitID, Date))")))
    {
        qDebug() << "[Error]: In HistoryManager::setup - unable to create visited table. Message: " << query.lastError().text();
    }
}

void HistoryManager::load()
{
    // Only load data from the History table, will load specific visits if user requests full history
    QSqlQuery query(m_database);

    // First clear history entries that are not referenced by any specific visits
    if (!query.exec(QLatin1String("DELETE FROM History WHERE VisitID NOT IN (SELECT DISTINCT VisitID FROM Visits)")))
    {
        qDebug() << "[Error]: In HistoryManager::load - Could not remove non-referenced history entries from the database. Message: " << query.lastError().text();
    }

    if (query.exec(QLatin1String("SELECT VisitID, URL, Title FROM History ORDER BY VisitID ASC")))
    {
        QSqlRecord rec = query.record();
        int idVisit = rec.indexOf(QLatin1String("VisitID"));
        int idUrl = rec.indexOf(QLatin1String("URL"));
        int idTitle = rec.indexOf(QLatin1String("Title"));
        while (query.next())
        {
            WebHistoryItem item;
            item.URL = query.value(idUrl).toString();
            item.Title = query.value(idTitle).toString();
            item.VisitID = query.value(idVisit).toInt();

            m_lastVisitID = item.VisitID;
            m_historyItems.insert(item.URL.toString(), item);
        }
    }
    else
        qDebug() << "[Error]: In HistoryManager::load - Unable to fetch history items from the database. Message: "
                 << query.lastError().text();

    // Load most recent visits
    if (query.exec(QLatin1String("SELECT Visits.Date, History.URL FROM Visits INNER JOIN History ON Visits.VisitID = History.VisitID ORDER BY Visits.Date DESC LIMIT 15")))
    {
        QSqlRecord rec = query.record();
        int idDate = rec.indexOf(QLatin1String("Date"));
        int idUrl = rec.indexOf(QLatin1String("URL"));
        while (query.next())
        {
            auto it = m_historyItems.find(query.value(idUrl).toString());
            if (it != m_historyItems.end())
            {
                QDateTime date = QDateTime::fromMSecsSinceEpoch(query.value(idDate).toULongLong());
                it->Visits.prepend(date);
                m_recentItems.push_front(*it);
            }
        }
    }
    else
        qDebug() << "Could not load visit date info. Message: " << query.lastError().text();
}

void HistoryManager::save()
{
}

void HistoryManager::saveVisit(const WebHistoryItem &item, const QDateTime &visitTime)
{
    if (m_queryHistoryItem == nullptr)
    {
        m_queryHistoryItem = new QSqlQuery(m_database);
        m_queryHistoryItem->prepare(QLatin1String("INSERT OR IGNORE INTO History(VisitID, URL, Title) VALUES(:visitId, :url, :title)"));

        m_queryVisit = new QSqlQuery(m_database);
        m_queryVisit->prepare(QLatin1String("INSERT INTO Visits(VisitID, Date) VALUES(:visitId, :date)"));
    }

    m_queryHistoryItem->bindValue(QLatin1String(":visitId"), item.VisitID);
    m_queryHistoryItem->bindValue(QLatin1String(":url"), item.URL.toString());
    m_queryHistoryItem->bindValue(QLatin1String(":title"), item.Title);
    if (!m_queryHistoryItem->exec())
        qDebug() << "[Error]: In HistoryManager::saveVisit - unable to save history item to database. Message: " << m_queryHistoryItem->lastError().text();

    m_queryVisit->bindValue(QLatin1String(":visitId"), item.VisitID);
    m_queryVisit->bindValue(QLatin1String(":date"), visitTime.toMSecsSinceEpoch());
    if (!m_queryVisit->exec())
        qDebug() << "[Error]: In HistoryManager::saveVisit - unable to save specific visit for URL " << item.URL.toString() << " at time " << visitTime.toString();

}
