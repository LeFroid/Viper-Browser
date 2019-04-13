#include "AdBlockLog.h"

#include <algorithm>

namespace adblock
{

AdBlockLog::AdBlockLog(QObject *parent) :
    QObject(parent),
    m_entries(),
    m_timerId(0)
{
    // Prune logs every 30 minutes
    m_timerId = startTimer(1000 * 60 * 30);
}

AdBlockLog::~AdBlockLog()
{
    killTimer(m_timerId);
}

void AdBlockLog::addEntry(FilterAction action, const QUrl &firstPartyUrl, const QUrl &requestUrl,
              ElementType resourceType, const QString &rule, const QDateTime &timestamp)
{
    auto it = m_entries.find(firstPartyUrl);
    if (it != m_entries.end())
        it->push_back({ action, firstPartyUrl, requestUrl, resourceType, rule, timestamp });
    else
        m_entries[firstPartyUrl] = { { action, firstPartyUrl, requestUrl, resourceType, rule, timestamp } };
}

std::vector<LogEntry> AdBlockLog::getAllEntries() const
{
    // Combine all entries
    std::vector<LogEntry> entries;
    for (auto it = m_entries.cbegin(); it != m_entries.cend(); ++it)
    {
        for (const LogEntry &logEntry : *it)
            entries.push_back(logEntry);
    }

    // Sort entries from newest to oldest
    std::sort(entries.begin(), entries.end(), [](const LogEntry &a, const LogEntry &b){
        return a.Timestamp < b.Timestamp;
    });

    return entries;
}

const std::vector<LogEntry> &AdBlockLog::getEntriesFor(const QUrl &firstPartyUrl)
{
    auto it = m_entries.find(firstPartyUrl);
    if (it != m_entries.end())
        return *it;

    std::vector<LogEntry> entriesForUrl;
    it = m_entries.insert(firstPartyUrl, entriesForUrl);
    return *it;
}

void AdBlockLog::timerEvent(QTimerEvent */*event*/)
{
    pruneLogs();
}

void AdBlockLog::pruneLogs()
{
    const quint64 pruneThreshold = 1000 * 60 * 30;
    const QDateTime now = QDateTime::currentDateTime();
    auto removeCheck = [&](const LogEntry &logEntry) {
        return static_cast<quint64>(logEntry.Timestamp.msecsTo(now)) >= pruneThreshold;
    };

    for (auto it = m_entries.begin(); it != m_entries.end(); ++it)
    {
        std::vector<LogEntry> &entries = *it;
        auto newEnd = std::remove_if(entries.begin(), entries.end(), removeCheck);
        entries.erase(newEnd, entries.end());
    }
}

}
