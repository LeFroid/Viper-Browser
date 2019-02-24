#include "URLRecord.h"

URLRecord::URLRecord(HistoryEntry &&entry, std::vector<VisitEntry> &&visits) :
    m_historyEntry(std::move(entry)),
    m_visits(std::move(visits))
{
}

const QDateTime &URLRecord::getLastVisit() const
{
    return m_historyEntry.LastVisit;
}

int URLRecord::getNumVisits() const
{
    return m_historyEntry.NumVisits;
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

void URLRecord::addVisit(VisitEntry &&entry)
{
    m_historyEntry.NumVisits++;

    if (entry.VisitTime > m_historyEntry.LastVisit)
        m_historyEntry.LastVisit = entry.VisitTime;

    m_visits.push_back(std::move(entry));
}
