#include "CommonUtil.h"
#include "HistoryManager.h"
#include "HistoryStore.h"
#include "Settings.h"

#include <algorithm>
#include <array>

#include <QDateTime>
#include <QUrl>

#include <QDebug>

HistoryManager::HistoryManager(const ViperServiceLocator &serviceLocator, DatabaseTaskScheduler &taskScheduler) :
    QObject(nullptr),
    m_taskScheduler(taskScheduler),
    m_historyItems(),
    m_recentItems(),
    m_storagePolicy(HistoryStoragePolicy::Remember),
    m_historyStore(nullptr),
    m_lastVisitId(0)
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
        //onHistoryRecordsLoaded(m_historyStore->getEntries());
        onRecentItemsLoaded(m_historyStore->getRecentItems());

        m_lastVisitId = m_historyStore->getLastVisitId();
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

    m_taskScheduler.post(&HistoryStore::clearAllHistory, std::ref(m_historyStore));
}

void HistoryManager::clearHistoryFrom(const QDateTime &start)
{
    clearHistoryInRange({start, QDateTime::currentDateTime()});
}

void HistoryManager::clearHistoryInRange(std::pair<QDateTime, QDateTime> range)
{
    m_taskScheduler.post([this, range](){
        m_recentItems.clear();

        m_historyStore->clearHistoryInRange(range);

        onRecentItemsLoaded(m_historyStore->getRecentItems());
        emit historyCleared();
    });

    auto visitRemover = [&range](VisitEntry v){
        return v >= range.first && v <= range.second;
    };

    for (auto it = m_historyItems.begin(); it != m_historyItems.end();)
    {
        URLRecord &record = it->second;
        std::vector<VisitEntry> &visits = record.m_visits;
        visits.erase(std::remove_if(visits.begin(), visits.end(), visitRemover), visits.end());

        VisitEntry lastVisit = record.getLastVisit();
        if (visits.empty() &&
                (!lastVisit.isValid() || (lastVisit >= range.first && lastVisit <= range.second)))
        {
            it = m_historyItems.erase(it);
            continue;
        }

        if (!visits.empty())
        {
            record.m_historyEntry.NumVisits = static_cast<int>(visits.size());
            record.m_historyEntry.LastVisit = visits.at(visits.size() - 1);
        }

        ++it;
    }
}

void HistoryManager::addVisit(const QUrl &url, const QString &title, const QDateTime &visitTime, const QUrl &requestedUrl, bool wasTypedByUser)
{
    if (m_storagePolicy == HistoryStoragePolicy::Never
            || url.toString(QUrl::FullyEncoded).startsWith(QLatin1String("data:"), Qt::CaseInsensitive))
        return;

    m_taskScheduler.post(&HistoryStore::addVisit, std::ref(m_historyStore), QUrl(url), QString(title),
                         QDateTime(visitTime), QUrl(requestedUrl), wasTypedByUser);

    if (!CommonUtil::doUrlsMatch(requestedUrl, url))
    {
        QDateTime visit = visitTime.addSecs(-1);
        addVisitToLocalStore(requestedUrl, title, visit, wasTypedByUser);
    }

    addVisitToLocalStore(url, title, visitTime, wasTypedByUser);

    while (m_recentItems.size() > 15)
        m_recentItems.pop_back();

    emit pageVisited(url, title);
}

void HistoryManager::addVisitToLocalStore(const QUrl &url, const QString &title, const QDateTime &visitTime, bool wasTypedByUser)
{
    VisitEntry visit = visitTime;

    auto it = m_historyItems.find(url.toString().toUpper());
    if (it != m_historyItems.end())
    {
        URLRecord &record = it->second;
        if (wasTypedByUser)
            record.m_historyEntry.URLTypedCount++;

        record.addVisit(visit);

        m_recentItems.push_front(record.m_historyEntry);
    }
    else
    {
        HistoryEntry entry;
        entry.VisitID = static_cast<int>(++m_lastVisitId);
        entry.NumVisits = 1;
        entry.LastVisit = visitTime;
        entry.Title = title;
        entry.URL = url;
        entry.URLTypedCount = wasTypedByUser ? 1 : 0;
        std::vector<VisitEntry> visits {visit};

        m_historyItems.insert(std::make_pair(url.toString().toUpper(), URLRecord(std::move(entry), std::move(visits))));
        m_recentItems.push_front(entry);
    }
}

void HistoryManager::getHistoryBetween(const QDateTime &startDate, const QDateTime &endDate, std::function<void(std::vector<URLRecord>)> callback)
{
    m_taskScheduler.post([this, startDate, endDate, callback](){
        callback(m_historyStore->getHistoryBetween(startDate, endDate));
    });
}

void HistoryManager::getHistoryFrom(const QDateTime &startDate, std::function<void(std::vector<URLRecord>)> callback)
{
    m_taskScheduler.post([this, startDate, callback](){
        callback(m_historyStore->getHistoryFrom(startDate));
    });
}

void HistoryManager::contains(const QUrl &url, std::function<void(bool)> callback)
{
    m_taskScheduler.post([this, url, callback](){
        callback(m_historyStore->contains(url));
    });
}

HistoryEntry HistoryManager::getEntry(const QUrl &url) const
{
    HistoryEntry result;

    auto it = m_historyItems.find(url.toString().toUpper());
    if (it != m_historyItems.end())
        result = it->second.m_historyEntry;

    return result;
}

void HistoryManager::getTimesVisitedHost(const QUrl &host, std::function<void(int)> callback)
{
    m_taskScheduler.post([this, host, callback](){
        callback(m_historyStore->getTimesVisitedHost(host));
    });
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
        m_historyItems.insert(std::make_pair(record.getUrl().toString().toUpper(), record));
    }
}

void HistoryManager::loadMostVisitedEntries(int limit, std::function<void(std::vector<WebPageInformation>)> callback)
{
    m_taskScheduler.post([this, limit, callback](){
        callback(m_historyStore->loadMostVisitedEntries(limit));
    });
}

void HistoryManager::loadWordDatabase(std::function<void(std::map<int, QString>)> callback)
{
    m_taskScheduler.post([this, callback](){
        callback(m_historyStore->getWords());
    });
}

void HistoryManager::loadHistoryWordMapping(std::function<void(std::map<int, std::vector<int>>)> callback)
{
    m_taskScheduler.post([this, callback](){
        callback(m_historyStore->getEntryWordMapping());
    });
}
