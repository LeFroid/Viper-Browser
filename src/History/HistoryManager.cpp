#include "BrowserApplication.h"
#include "HistoryManager.h"
#include "Settings.h"
#include "WebView.h"
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

HistoryManager::HistoryManager(const QString &databaseFile, QObject *parent) :
    QObject(parent),
    DatabaseWorker(databaseFile, QLatin1String("HistoryDB")),
    m_lastVisitID(0),
    m_historyItems(),
    m_recentItems(),
    m_queryHistoryItem(nullptr),
    m_queryVisit(nullptr),
    m_storagePolicy(HistoryStoragePolicy::Remember),
    m_mutex()
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
        m_queryHistoryItem = nullptr;

        delete m_queryVisit;
        m_queryVisit = nullptr;
    }
}

void HistoryManager::clearAllHistory()
{
    {
        std::lock_guard<std::mutex> _(m_mutex);

        if (!exec(QLatin1String("DELETE FROM History")))
            qDebug() << "[Error]: In HistoryManager::clearAllHistory - Unable to clear History table.";

        if (!exec(QLatin1String("DELETE FROM Visits")))
            qDebug() << "[Error]: In HistoryManager::clearAllHistory - Unable to clear Visits table.";
    }
    m_recentItems.clear();
    m_historyItems.clear();
}

void HistoryManager::clearHistoryFrom(const QDateTime &start)
{
    {
        std::lock_guard<std::mutex> _(m_mutex);

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
    }

    m_recentItems.clear();
    m_historyItems.clear();
    load();
}

void HistoryManager::clearHistoryInRange(std::pair<QDateTime, QDateTime> range)
{
    {
        std::lock_guard<std::mutex> _(m_mutex);

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
    }

    m_recentItems.clear();
    m_historyItems.clear();
    load();
}

bool HistoryManager::historyContains(const QString &url) const
{
    return (m_historyItems.find(url.toUpper()) != m_historyItems.end());
}

WebHistoryItem HistoryManager::getEntry(const QUrl &url) const
{
    WebHistoryItem result;

    auto it = m_historyItems.find(url.toString().toUpper());
    if (it != m_historyItems.end())
        result = *it;

    return result;
}

std::vector<WebHistoryItem> HistoryManager::getHistoryFrom(const QDateTime &startDate) const
{
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
        item.URL = queryHistoryItem.value(0).toUrl();
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

int HistoryManager::getTimesVisitedHost(const QString &host) const
{
    int timesVisited = 0;

    QSqlQuery query(m_database);
    query.prepare(QLatin1String("SELECT COUNT(VisitID) FROM Visits WHERE VisitID = (:id)"));
    for (const WebHistoryItem &item : m_historyItems)
    {
        if (host.endsWith(item.URL.host().remove(QRegularExpression("(http(s)?://)?(www\\.)"))))
        {
            query.bindValue(QLatin1String(":id"), item.VisitID);
            if (query.exec() && query.next())
                timesVisited += query.value(0).toInt();
        }
    }
    return timesVisited;
}

int HistoryManager::getTimesVisited(const QUrl &url) const
{
    QSqlQuery queryVisitId(m_database);
    queryVisitId.prepare(QLatin1String("SELECT VisitID FROM History WHERE URL = (:url)"));
    queryVisitId.bindValue(QLatin1String(":url"), url);

    if (queryVisitId.exec() && queryVisitId.first())
    {
        int visitId = queryVisitId.value(0).toInt();

        QSqlQuery queryVisitCount(m_database);
        queryVisitCount.prepare(QLatin1String("SELECT COUNT(VisitID) FROM Visits WHERE VisitID = (:id)"));
        queryVisitCount.bindValue(QLatin1String(":id"), visitId);

        if (queryVisitCount.exec() && queryVisitCount.first())
            return queryVisitCount.value(0).toInt();
    }

    return 0;
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

void HistoryManager::onPageLoaded(bool ok)
{
    if (!ok || m_storagePolicy == HistoryStoragePolicy::Never)
        return;

    WebWidget *ww = qobject_cast<WebWidget*>(sender());
    if (ww == nullptr || ww->isOnBlankPage())
        return;

    const QUrl url = ww->url();
    const QUrl originalUrl = ww->getOriginalUrl();
    const QString scheme = url.scheme().toLower();

    // Check if we should ignore this page
    const std::array<QString, 2> ignoreSchemes { QLatin1String("qrc"), QLatin1String("viper") };
    if (url.isEmpty() || originalUrl.isEmpty() || scheme.isEmpty())
        return;

    for (const QString &ignoreScheme : ignoreSchemes)
    {
        if (scheme == ignoreScheme)
            return;
    }

    // Find or create a new history entry
    const QDateTime visitTime = QDateTime::currentDateTime();
    const QString title = ww->getTitle();
    const bool emptyTitle = title.isEmpty();

    std::vector<QUrl> urls { originalUrl };
    if (originalUrl.adjusted(QUrl::RemoveScheme) != url.adjusted(QUrl::RemoveScheme))
        urls.push_back(url);

    for (const QUrl &currentUrl : urls)
    {
        QString urlFormatted = currentUrl.toString();
        const QString urlUpper = urlFormatted.toUpper();

        auto it = m_historyItems.find(urlUpper);
        if (it != m_historyItems.end())
        {
            it->Visits.prepend(visitTime);
            if (it->Title.isEmpty() && !emptyTitle)
                it->Title = title;

            m_recentItems.push_front(*it);
            while (m_recentItems.size() > 15)
                m_recentItems.pop_back();

            if (!it->Title.isEmpty())
                QtConcurrent::run(this, &HistoryManager::saveVisit, *it, visitTime);
        }
        else
        {
            WebHistoryItem item;
            item.URL = currentUrl;
            item.VisitID = ++m_lastVisitID;
            item.Title = title;
            item.Visits.prepend(visitTime);

            m_historyItems.insert(urlUpper, item);
            m_recentItems.push_front(item);
            while (m_recentItems.size() > 15)
                m_recentItems.pop_back();

            QtConcurrent::run(this, &HistoryManager::saveVisit, item, visitTime);
        }
    }

    emit pageVisited(url.toString(), title);
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
    purgeOldEntries();

    // Only load data from the History table, will load specific visits if user requests full history
    QSqlQuery query(m_database);

    // Clear history entries that are not referenced by any specific visits
    if (!query.exec(QLatin1String("DELETE FROM History WHERE VisitID NOT IN (SELECT DISTINCT VisitID FROM Visits)")))
    {
        qWarning() << "In HistoryManager::load - Could not remove non-referenced history entries from the database."
                   << " Message: " << query.lastError().text();
    }

    // Now load history entries
    if (!query.exec(QLatin1String("SELECT VisitID, URL, Title FROM History ORDER BY VisitID ASC")))
    {
        qWarning() << "In HistoryManager::load - Unable to fetch history items from the database. Message: "
                   << query.lastError().text();
    }
    else
    {
        QSqlRecord rec = query.record();
        int idVisit = rec.indexOf(QLatin1String("VisitID"));
        int idUrl = rec.indexOf(QLatin1String("URL"));
        int idTitle = rec.indexOf(QLatin1String("Title"));
        while (query.next())
        {
            WebHistoryItem item;
            item.URL = query.value(idUrl).toUrl();
            item.Title = query.value(idTitle).toString();
            item.VisitID = query.value(idVisit).toInt();

            m_lastVisitID = item.VisitID;
            m_historyItems.insert(item.URL.toString().toUpper(), item);
        }
    }

    // Load most recent visits
    loadRecentVisits();
}

void HistoryManager::save()
{
}

void HistoryManager::saveVisit(const WebHistoryItem &item, const QDateTime &visitTime)
{
    std::lock_guard<std::mutex> _(m_mutex);

    if (m_queryHistoryItem == nullptr)
    {
        m_queryHistoryItem = new QSqlQuery(m_database);
        m_queryHistoryItem->prepare(QLatin1String("INSERT OR IGNORE INTO History(VisitID, URL, Title) VALUES(:visitId, :url, :title)"));

        m_queryVisit = new QSqlQuery(m_database);
        m_queryVisit->prepare(QLatin1String("INSERT INTO Visits(VisitID, Date) VALUES(:visitId, :date)"));
    }

    m_queryHistoryItem->bindValue(QLatin1String(":visitId"), item.VisitID);
    m_queryHistoryItem->bindValue(QLatin1String(":url"), item.URL);
    m_queryHistoryItem->bindValue(QLatin1String(":title"), item.Title);
    if (!m_queryHistoryItem->exec())
        qDebug() << "[Error]: In HistoryManager::saveVisit - unable to save history item to database. Message: " << m_queryHistoryItem->lastError().text();

    m_queryVisit->bindValue(QLatin1String(":visitId"), item.VisitID);
    m_queryVisit->bindValue(QLatin1String(":date"), visitTime.toMSecsSinceEpoch());
    if (!m_queryVisit->exec())
        qDebug() << "[Error]: In HistoryManager::saveVisit - unable to save specific visit for URL " << item.URL.toString() << " at time " << visitTime.toString();
}

void HistoryManager::purgeOldEntries()
{
    // Clear visits that are 6+ months old
    QSqlQuery query(m_database);
    quint64 purgeDate = QDateTime::currentMSecsSinceEpoch();
    const quint64 tmp = quint64{15552000000};
    if (purgeDate > tmp)
    {
        purgeDate -= tmp;
        query.prepare(QLatin1String("DELETE FROM Visits WHERE Date < (:purgeDate)"));
        query.bindValue(QLatin1String(":purgeDate"), purgeDate);
        if (!query.exec())
        {
            qWarning() << "HistoryManager - Could not purge old history entries. Message: " << query.lastError().text();
        }
    }
}

void HistoryManager::loadRecentVisits()
{
    QSqlQuery query(m_database);
    if (!query.exec(QLatin1String("SELECT Visits.Date, History.URL FROM Visits"
                                  " INNER JOIN History ON Visits.VisitID = History.VisitID"
                                  " ORDER BY Visits.Date DESC LIMIT 15")))
    {
        qWarning() << "HistoryManager - could not load recently visited history entries";
        return;
    }

    QSqlRecord rec = query.record();
    int idDate = rec.indexOf(QLatin1String("Date"));
    int idUrl = rec.indexOf(QLatin1String("URL"));
    while (query.next())
    {
        auto it = m_historyItems.find(query.value(idUrl).toString().toUpper());
        if (it != m_historyItems.end())
        {
            QDateTime date = QDateTime::fromMSecsSinceEpoch(query.value(idDate).toULongLong());
            it->Visits.prepend(date);
            m_recentItems.push_back(*it);
        }
    }
}

std::vector<WebPageInformation> HistoryManager::loadMostVisitedEntries(int limit)
{
    //m_mostVisitedList.clear();
    std::vector<WebPageInformation> result;
    if (limit <= 0)
        return result;

    QSqlQuery query(m_database);
    if (!query.exec(QString("SELECT v.VisitID, COUNT(v.VisitID) AS NumVisits, h.URL, h.Title FROM Visits AS v JOIN History AS h"
                            " ON v.VisitID = h.VisitID GROUP BY v.VisitID ORDER BY NumVisits DESC LIMIT %1").arg(limit)))
    {
        qWarning() << "In HistoryManager::loadMostVisitedEntries - unable to load most frequently visited entries. Message: " << query.lastError().text();
        return result;
    }

    QSqlRecord rec = query.record();
    int idUrl = rec.indexOf(QLatin1String("URL"));
    int idTitle = rec.indexOf(QLatin1String("Title"));
    while (query.next())
    {
        WebPageInformation item;
        item.Position = 0;
        item.URL = query.value(idUrl).toUrl();
        item.Title = query.value(idTitle).toString();
        result.push_back(item);
    }

    return result;
}

