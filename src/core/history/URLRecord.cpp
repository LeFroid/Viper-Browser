#include "URLRecord.h"

URLRecord::URLRecord(HistoryEntry &&entry, std::vector<VisitEntry> &&visits) noexcept :
    m_historyEntry(std::move(entry)),
    m_visits(std::move(visits))
{
}

URLRecord::URLRecord(HistoryEntry &&entry) noexcept :
    m_historyEntry(std::move(entry)),
    m_visits()
{
}

const VisitEntry &URLRecord::getLastVisit() const
{
    return m_historyEntry.LastVisit;
}

int URLRecord::getNumVisits() const
{
    return m_historyEntry.NumVisits;
}

int URLRecord::getUrlTypedCount() const
{
    return m_historyEntry.URLTypedCount;
}

const QUrl &URLRecord::getUrl() const
{
    return m_historyEntry.URL;
}

const QString &URLRecord::getTitle() const
{
    return m_historyEntry.Title;
}

int URLRecord::getVisitId() const
{
    return m_historyEntry.VisitID;
}

const std::vector<VisitEntry> &URLRecord::getVisits() const
{
    return m_visits;
}

void URLRecord::addVisit(VisitEntry entry)
{
    m_historyEntry.NumVisits++;

    if (entry > m_historyEntry.LastVisit)
        m_historyEntry.LastVisit = entry;

    m_visits.push_back(entry);
}
