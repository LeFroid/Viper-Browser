#include "HistoryManager.h"

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
    QWebHistoryInterface(parent),
    DatabaseWorker(databaseFile, QStringLiteral("HistoryDB")),
    m_lastVisitID(0),
    m_historyItems(),
    m_recentItems(),
    m_queryHistoryItem(nullptr),
    m_queryVisit(nullptr)
{
    QWebHistoryInterface::setDefaultInterface(this);
}

HistoryManager::~HistoryManager()
{
    if (m_queryHistoryItem != nullptr)
    {
        delete m_queryHistoryItem;
        delete m_queryVisit;
    }
}

void HistoryManager::addHistoryEntry(const QString &url)
{
    // Check if qrc:/ resource, and if so, ignore
    if (url.startsWith("qrc:"))
        return;

    QDateTime visitTime = QDateTime::currentDateTime();

    auto it = m_historyItems.find(url);
    if (it != m_historyItems.end())
    {
        it->Visits.prepend(visitTime);
        m_recentItems.push_front(*it);
        while (m_recentItems.size() > 15)
            m_recentItems.pop_back();
        if (!it->Title.isEmpty())
        {
            saveVisit(*it, visitTime);
            emit pageVisited(url, it->Title);
        }
        return;
    }

    WebHistoryItem item;
    item.URL = url;
    item.VisitID = ++m_lastVisitID;
    item.Visits.prepend(visitTime);
    m_historyItems.insert(url, item);
    m_recentItems.push_front(item);
    while (m_recentItems.size() > 15)
        m_recentItems.pop_back();
    // don't emit pageLoaded until title is set
}

void HistoryManager::clearAllHistory()
{
    if (!exec(QStringLiteral("DELETE FROM History")))
        qDebug() << "[Error]: In HistoryManager::clearAllHistory - Unable to clear History table.";

    if (!exec(QStringLiteral("DELETE FROM Visits")))
        qDebug() << "[Error]: In HistoryManager::clearAllHistory - Unable to clear Visits table.";

    m_recentItems.clear();
    m_historyItems.clear();
}

void HistoryManager::clearHistoryFrom(const QDateTime &start)
{
    // Perform database query and reload data
    QSqlQuery query(m_database);
    query.prepare("DELETE FROM Visits WHERE Date > (:date)");
    query.bindValue(":date", start.toMSecsSinceEpoch());
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
    query.prepare("DELETE FROM Visits WHERE Date > (:startDate) AND Date < (:endDate)");
    query.bindValue(":startDate", range.first.toMSecsSinceEpoch());
    query.bindValue(":endDate", range.second.toMSecsSinceEpoch());
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

void HistoryManager::setTitleForURL(const QString &url, const QString &title)
{
    auto it = m_historyItems.find(url);
    if (it != m_historyItems.end())
    {
        if (it->Title.isEmpty())
        {
            it->Title = title;

            int recentVisitIdx = -1;
            for (size_t i = 0; i < m_recentItems.size(); ++i)
            {
                if (m_recentItems.at(i) == *it)
                {
                    recentVisitIdx = static_cast<int>(i);
                    break;
                }
            }
            if (recentVisitIdx >= 0)
                m_recentItems[recentVisitIdx].Title = title;

            saveVisit(*it, it->Visits.front());
            emit pageVisited(url, title);
        }
    }
}

std::vector<WebHistoryItem> HistoryManager::getHistoryFrom(const QDateTime &startDate) const
{
    std::vector<WebHistoryItem> items;

    if (!startDate.isValid())
        return items;

    // Get startDate in msec format
    qint64 startMSec = startDate.toMSecsSinceEpoch();

    // Prepare DB query (date is in MSecsSinceEpoch())
    QSqlQuery query(m_database);
    query.prepare("SELECT Date FROM Visits WHERE VisitID = (:id) AND Date >= (:date)");

    // Iterate through list of all visited URLs, searching the DB to confirm the date is between startDate and now
    for (const auto &it : m_historyItems)
    {
        WebHistoryItem item;
        item.Title = it.Title;
        item.URL = it.URL;
        item.VisitID = it.VisitID;
        //item.Visits = it.Visits;

        // Check DB
        query.bindValue(":id", it.VisitID);
        query.bindValue(":date", startMSec);
        if (query.exec())
        {
            while (query.next())
                item.Visits.append(QDateTime::fromMSecsSinceEpoch(query.value(0).toLongLong()));
        }
        items.push_back(item);
    }
    return items;
}

int HistoryManager::getTimesVisited(const QString &host)
{
    QSqlQuery query(m_database);
    query.prepare("SELECT COUNT(VisitID) FROM Visits WHERE VisitID = (:id)");
    int timesVisited = 0;
    for (const WebHistoryItem &item : m_historyItems)
    {
        if (host.endsWith(item.URL.host().remove(QRegExp("(http(s)?://)?(www.)"))))
        {
            query.bindValue(":id", item.VisitID);
            if (query.exec() && query.next())
                timesVisited += query.value(0).toInt();
        }
    }
    return timesVisited;
}

bool HistoryManager::hasProperStructure()
{
    // Verify existence of Visits and History tables
    return hasTable(QStringLiteral("Visits")) && hasTable(QStringLiteral("History"));
}

void HistoryManager::setup()
{
    QSqlQuery query(m_database);
    if (!query.exec("CREATE TABLE IF NOT EXISTS Visits(VisitID INTEGER NOT NULL, Date INTEGER NOT NULL, "
                    "PRIMARY KEY(VisitID, Date))"))
    {
        qDebug() << "[Error]: In HistoryManager::setup - unable to create visited table. Message: " << query.lastError().text();
        return;
    }
    if (!query.exec("CREATE TABLE IF NOT EXISTS History(URL TEXT PRIMARY KEY, Title TEXT, VisitID INTEGER NOT NULL, "
                    "FOREIGN KEY(VisitID) REFERENCES Visits(VisitID) ON DELETE CASCADE)"))
    {
        qDebug() << "[Error]: In HistoryManager::setup - unable to create history table. Message: " << query.lastError().text();
    }
}

void HistoryManager::load()
{
    // Only load data from the History table, will load specific visits if user requests full history
    QSqlQuery query(m_database);

    if (query.exec("SELECT URL, TITLE, VisitID FROM History ORDER BY VisitID ASC"))
    {
        QSqlRecord rec = query.record();
        int idUrl = rec.indexOf("URL");
        int idTitle = rec.indexOf("Title");
        int idVisit = rec.indexOf("VisitID");
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
    if (query.exec("SELECT Visits.Date, History.URL FROM Visits INNER JOIN History ON Visits.VisitID = History.VisitID ORDER BY Visits.Date DESC LIMIT 15"))
    {
        QSqlRecord rec = query.record();
        int idDate = rec.indexOf("Date");
        int idUrl = rec.indexOf("URL");
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
        m_queryHistoryItem->prepare("INSERT OR IGNORE INTO History(URL, Title, VisitID) VALUES(:url, :title, :visitId)");

        m_queryVisit = new QSqlQuery(m_database);
        m_queryVisit->prepare("INSERT INTO Visits(VisitID, Date) VALUES(:visitId, :date)");
    }

    m_queryHistoryItem->bindValue(":url", item.URL.toString());
    m_queryHistoryItem->bindValue(":title", item.Title);
    m_queryHistoryItem->bindValue(":visitId", item.VisitID);
    if (!m_queryHistoryItem->exec())
        qDebug() << "[Error]: In HistoryManager::saveVisit - unable to save history item to database. Message: " << m_queryHistoryItem->lastError().text();

    m_queryVisit->bindValue(":visitId", item.VisitID);
    m_queryVisit->bindValue(":date", visitTime.toMSecsSinceEpoch());
    if (!m_queryVisit->exec())
        qDebug() << "[Error]: In HistoryManager::saveVisit - unable to save specific visit for URL " << item.URL.toString() << " at time " << visitTime.toString();

}
