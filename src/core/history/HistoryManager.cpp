#include "CommonUtil.h"
#include "HistoryManager.h"
#include "HistoryStore.h"
#include "Settings.h"

#include <algorithm>
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

HistoryManager::HistoryManager(const ViperServiceLocator &serviceLocator, DatabaseTaskScheduler &taskScheduler) :
    QObject(nullptr),
    m_taskScheduler(taskScheduler),
    m_historyItems(),
    m_recentItems(),
    m_storagePolicy(HistoryStoragePolicy::Remember),
    m_historyStore(nullptr)
{
    setObjectName(QLatin1String("HistoryManager"));

    if (Settings *settings = serviceLocator.getServiceAs<Settings>("Settings"))
    {
        m_storagePolicy = static_cast<HistoryStoragePolicy>(settings->getValue(BrowserSetting::HistoryStoragePolicy).toInt());

        connect(settings, &Settings::settingChanged, this, &HistoryManager::onSettingChanged);
    }
    else
        qWarning() << "Could not fetch application settings in history manager!";

    m_taskScheduler.onInit([this](){
        m_historyStore = static_cast<HistoryStore*>(m_taskScheduler.getWorker("HistoryStore"));
    });

    m_taskScheduler.post([this](){
        onHistoryRecordsLoaded(m_historyStore->getEntries());
        onRecentItemsLoaded(m_historyStore->getRecentItems());
    });
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
}

void HistoryManager::clearAllHistory()
{
    m_recentItems.clear();
    m_historyItems.clear();

    m_taskScheduler.post([this](){
        m_historyStore->clearAllHistory();
    });
}

void HistoryManager::clearHistoryFrom(const QDateTime &start)
{
    clearHistoryInRange({start, QDateTime::currentDateTime()});
}

void HistoryManager::clearHistoryInRange(std::pair<QDateTime, QDateTime> range)
{
    m_taskScheduler.post([=](){
        m_recentItems.clear();

        m_historyStore->clearHistoryInRange(range);

        onRecentItemsLoaded(m_historyStore->getRecentItems());
        emit historyCleared();
    });

    for (auto it = m_historyItems.begin(); it != m_historyItems.end();)
    {
        std::vector<VisitEntry> &visits = it->m_visits;
        visits.erase(std::remove_if(visits.begin(), visits.end(), [&range](VisitEntry v){
            return v >= range.first && v <= range.second;
        }), visits.end());

        if (visits.empty())
        {
            it = m_historyItems.erase(it);
            continue;
        }

        it->m_historyEntry.NumVisits = static_cast<int>(visits.size());
        it->m_historyEntry.LastVisit = visits.at(visits.size() - 1);
        ++it;
    }
}

void HistoryManager::addVisit(const QUrl &url, const QString &title, const QDateTime &visitTime, const QUrl &requestedUrl, bool wasTypedByUser)
{
    if (m_storagePolicy == HistoryStoragePolicy::Never)
        return;

    m_taskScheduler.post([=](){
        m_historyStore->addVisit(url, title, visitTime, requestedUrl, wasTypedByUser);
    });

    VisitEntry visit = visitTime;

    auto it = m_historyItems.find(url.toString().toUpper());
    if (it != m_historyItems.end())
    {
        if (wasTypedByUser)
            it->m_historyEntry.URLTypedCount++;

        it->addVisit(visit);

        m_recentItems.push_front(it->m_historyEntry);
    }
    else
    {
        HistoryEntry entry;
        entry.VisitID = -1;
        entry.NumVisits = 1;
        entry.LastVisit = visitTime;
        entry.Title = title;
        entry.URL = url;
        entry.URLTypedCount = wasTypedByUser ? 1 : 0;
        std::vector<VisitEntry> visits {visit};
        m_historyItems.insert(url.toString().toUpper(), URLRecord(std::move(entry), std::move(visits)));
        m_recentItems.push_front(entry);
    }

    if (!CommonUtil::doUrlsMatch(requestedUrl, url))
    {
        visit = visitTime.addSecs(-1);

        it = m_historyItems.find(requestedUrl.toString().toUpper());
        if (it != m_historyItems.end())
        {
            if (wasTypedByUser)
                it->m_historyEntry.URLTypedCount++;

            it->addVisit(visit);
            m_recentItems.push_front(it->m_historyEntry);
        }
        else
        {
            HistoryEntry entry;
            entry.VisitID = -1;
            entry.NumVisits = 1;
            entry.LastVisit = visitTime;
            entry.Title = title;
            entry.URL = requestedUrl;
            entry.URLTypedCount = wasTypedByUser ? 1 : 0;
            std::vector<VisitEntry> visits {visit};
            m_historyItems.insert(requestedUrl.toString().toUpper(), URLRecord(std::move(entry), std::move(visits)));
            m_recentItems.push_front(entry);
        }
    }

    while (m_recentItems.size() > 15)
        m_recentItems.pop_back();

    emit pageVisited(url, title);
}

void HistoryManager::getHistoryBetween(const QDateTime &startDate, const QDateTime &endDate, std::function<void(std::vector<URLRecord>)> callback)
{
    m_taskScheduler.post([=](){
        callback(m_historyStore->getHistoryBetween(startDate, endDate));
    });
}

void HistoryManager::getHistoryFrom(const QDateTime &startDate, std::function<void(std::vector<URLRecord>)> callback)
{
    m_taskScheduler.post([=](){
        callback(m_historyStore->getHistoryFrom(startDate));
    });
}

bool HistoryManager::contains(const QUrl &url) const
{
    return (m_historyItems.find(url.toString().toUpper()) != m_historyItems.end());
}

HistoryEntry HistoryManager::getEntry(const QUrl &url) const
{
    HistoryEntry result;

    auto it = m_historyItems.find(url.toString().toUpper());
    if (it != m_historyItems.end())
        result = it->m_historyEntry;

    return result;
}

void HistoryManager::getTimesVisitedHost(const QUrl &host, std::function<void(int)> callback)
{
    m_taskScheduler.post([=](){
        callback(m_historyStore->getTimesVisitedHost(host));
    });
}

int HistoryManager::getTimesVisited(const QUrl &url) const
{
    auto it = m_historyItems.find(url.toString().toUpper());
    if (it != m_historyItems.end())
        return it->getNumVisits();

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
        clearAllHistory();
}

void HistoryManager::onSettingChanged(BrowserSetting setting, const QVariant &value)
{
    if (setting == BrowserSetting::HistoryStoragePolicy)
        setStoragePolicy(static_cast<HistoryStoragePolicy>(value.toInt()));
}

void HistoryManager::onRecentItemsLoaded(std::deque<HistoryEntry> &&entries)
{
    m_recentItems = std::move(entries);
}

void HistoryManager::onHistoryRecordsLoaded(std::vector<URLRecord> &&records)
{
    m_historyItems.clear();

    for (auto &&record : records)
    {
        m_historyItems.insert(record.getUrl().toString().toUpper(), std::move(record));
    }
}

void HistoryManager::loadMostVisitedEntries(int limit, std::function<void(std::vector<WebPageInformation>)> callback)
{
    m_taskScheduler.post([=](){
        callback(m_historyStore->loadMostVisitedEntries(limit));
    });
}
