#include "HistoryManager.h"
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

HistoryManager::HistoryManager(const ViperServiceLocator &serviceLocator, DatabaseTaskScheduler &taskScheduler) :
    QObject(nullptr),
    m_taskScheduler(taskScheduler),
    m_lastVisitID(0),
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

    //m_threadManager.initHistoryStore(databaseFile);
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
    m_taskScheduler.post([=](){
        m_historyItems.clear();
        m_recentItems.clear();

        m_historyStore->clearHistoryFrom(start);

        m_recentItems = m_historyStore->getRecentItems();
        emit historyCleared();
    });
}

void HistoryManager::clearHistoryInRange(std::pair<QDateTime, QDateTime> range)
{   
    m_taskScheduler.post([=](){
        m_historyItems.clear();
        m_recentItems.clear();

        m_historyStore->clearHistoryInRange(range);

        m_recentItems = m_historyStore->getRecentItems();
        emit historyCleared();
    });
}

void HistoryManager::addVisit(const QUrl &url, const QString &title, const QDateTime &visitTime, const QUrl &requestedUrl)
{
    m_taskScheduler.post([=](){
        m_historyStore->addVisit(url, title, visitTime, requestedUrl);
    });

    VisitEntry visit;
    visit.VisitTime = visitTime;

    auto it = m_historyItems.find(url.toString().toUpper());
    if (it != m_historyItems.end())
    {
        visit.VisitID = it->getVisitId();
        it->addVisit(std::move(visit));

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
        std::vector<VisitEntry> visits {visit};
        m_historyItems.insert(url.toString().toUpper(), URLRecord(std::move(entry), std::move(visits)));
        m_recentItems.push_front(entry);
    }

    if (!url.matches(requestedUrl, QUrl::RemoveScheme | QUrl::RemoveAuthority))
    {
        visit.VisitID = -1;
        visit.VisitTime = visitTime;

        it = m_historyItems.find(requestedUrl.toString().toUpper());
        if (it != m_historyItems.end())
        {
            visit.VisitID = it->getVisitId();
            it->addVisit(std::move(visit));
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
            std::vector<VisitEntry> visits {visit};
            m_historyItems.insert(requestedUrl.toString().toUpper(), URLRecord(std::move(entry), std::move(visits)));
            m_recentItems.push_front(entry);
        }
    }

    while (m_recentItems.size() > 15)
        m_recentItems.pop_back();
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

bool HistoryManager::historyContains(const QString &url) const
{
    return (m_historyItems.find(url.toUpper()) != m_historyItems.end());
}

HistoryEntry HistoryManager::getEntry(const QUrl &url) const
{
    HistoryEntry result;

    auto it = m_historyItems.find(url.toString().toUpper());
    if (it != m_historyItems.end())
        result = it->m_historyEntry;

    return result;
}

void HistoryManager::getTimesVisitedHost(const QString &host, std::function<void(int)> callback)
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

void HistoryManager::onPageLoaded(bool ok)
{
    if (!ok || m_storagePolicy == HistoryStoragePolicy::Never)
        return;

    WebWidget *ww = qobject_cast<WebWidget*>(sender());
    if (!ww)
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
    addVisit(url, title, visitTime, originalUrl);

    emit pageVisited(url, title);
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
