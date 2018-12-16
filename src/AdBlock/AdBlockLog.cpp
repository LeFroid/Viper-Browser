#include "AdBlockLog.h"

#include <algorithm>

AdBlockLog::AdBlockLog(QObject *parent) :
    QObject(parent),
    m_entries(),
    m_timer()
{
    connect(&m_timer, &QTimer::timeout, this, &AdBlockLog::pruneLogs);

    // Prune logs every 30 minutes
    m_timer.start(1000 * 60 * 30);
}

void AdBlockLog::addEntry(AdBlockFilterAction action, const QUrl &firstPartyUrl, const QUrl &requestUrl,
              ElementType resourceType, const QString &rule, const QDateTime &timestamp)
{
    auto it = m_entries.find(firstPartyUrl);
    if (it != m_entries.end())
        it->push_back({ action, firstPartyUrl, requestUrl, resourceType, rule, timestamp });
    else
        m_entries[firstPartyUrl] = { { action, firstPartyUrl, requestUrl, resourceType, rule, timestamp } };
}

std::vector<AdBlockLogEntry> AdBlockLog::getAllEntries() const
{
    // Combine all entries
    std::vector<AdBlockLogEntry> entries;
    for (auto it = m_entries.cbegin(); it != m_entries.cend(); ++it)
    {
        for (const AdBlockLogEntry &logEntry : *it)
            entries.push_back(logEntry);
    }

    // Sort entries from newest to oldest
    std::sort(entries.begin(), entries.end(), [](const AdBlockLogEntry &a, const AdBlockLogEntry &b){
        return a.Timestamp < b.Timestamp;
    });

    return entries;
}

const std::vector<AdBlockLogEntry> &AdBlockLog::getEntriesFor(const QUrl &firstPartyUrl)
{
    auto it = m_entries.find(firstPartyUrl);
    if (it != m_entries.end())
        return *it;

    std::vector<AdBlockLogEntry> entriesForUrl;
    it = m_entries.insert(firstPartyUrl, entriesForUrl);
    return *it;
}

void AdBlockLog::pruneLogs()
{
    const quint64 pruneThreshold = 1000 * 60 * 30;
    const QDateTime now = QDateTime::currentDateTime();
    auto removeCheck = [&](const AdBlockLogEntry &logEntry) {
        return static_cast<quint64>(logEntry.Timestamp.msecsTo(now)) >= pruneThreshold;
    };

    for (auto it = m_entries.begin(); it != m_entries.end(); ++it)
    {
        std::vector<AdBlockLogEntry> &entries = *it;
        auto newEnd = std::remove_if(entries.begin(), entries.end(), removeCheck);
        entries.erase(newEnd, entries.end());
    }
}
