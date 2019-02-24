#ifndef HISTORYSTORE_H
#define HISTORYSTORE_H

#include "ClearHistoryOptions.h"
#include "DatabaseWorker.h"
#include "FavoritePagesManager.h"
#include "ServiceLocator.h"
#include "URLRecord.h"

#include <QDateTime>
#include <QHash>
#include <QIcon>
#include <QList>
#include <QMetaType>
#include <QUrl>

#include <deque>
#include <vector>

/**
 * @class HistoryStore
 * @brief Maintains the state of the browsing history that belongs to a user profile
 */
class HistoryStore : public DatabaseWorker
{
    friend class DatabaseFactory;

public:
    /// Constructs the history manager, given the path to the history database
    explicit HistoryStore(const QString &databaseFile);

    /// Destructor 
    ~HistoryStore();

    /// Clears all browsing history
    void clearAllHistory();

    /// Clears history stored from the given start time to the present
    void clearHistoryFrom(const QDateTime &start);

    /// Clears history within the given {start,end} date-time pair
    void clearHistoryInRange(std::pair<QDateTime, QDateTime> range);

    /// Returns true if the history contains the given url, false if else. Will return
    /// false if private browsing mode is enabled
    bool contains(const QUrl &url) const;

    /// Returns a history record corresponding to the given URL, or an empty record if it was not found in the
    /// database
    HistoryEntry getEntry(const QUrl &url);

    /// Returns a queue of recently visited items, with the most recent visits being at the front of the queue
    std::deque<HistoryEntry> getRecentItems();

    /// Loads and returns a list of all \ref HistoryEntry items visited from the given start date to the present
    std::vector<URLRecord> getHistoryFrom(const QDateTime &startDate) const;

    /// Loads and returns a list of all \ref HistoryEntry items visited between the given start date and end dates
    std::vector<URLRecord> getHistoryBetween(const QDateTime &startDate, const QDateTime &endDate) const;

    /// Returns the number of times the user has visited the given website by its hostname
    int getTimesVisitedHost(const QUrl &url) const;

    /// Returns the number of times that the given URL has been visited
    int getTimesVisited(const QUrl &url) const;

    /// Fetches the set of most frequently visited web pages, up to the given limit. This is used to
    /// determine which web pages' thumbnails to retrieve for the "New Tab" page
    std::vector<WebPageInformation> loadMostVisitedEntries(int limit = 10);

    /// Adds an entry to the history data store, given the URL, page title, time of visit, and the requested URL
    void addVisit(const QUrl &url, const QString &title, const QDateTime &visitTime, const QUrl &requestedUrl);

protected:
    /// Returns true if the history database contains the table structures needed for it to function properly,
    /// false if else.
    bool hasProperStructure() override;

    /// Creates the browsing history-related tables in the database
    void setup() override;

    /// Loads browsing history from the database
    void load() override;

    /// Saves browsing history into the database
    void save() override;

private:
    /// Removes history items that are more than six months old
    void purgeOldEntries();

private:
    /// Stores the last visit ID that has been used to record browsing history. Auto increments for each new history item
    uint64_t m_lastVisitID;

    /// Contains the history entries at the time of the initial database load
    std::vector<HistoryEntry> m_entries;

    /// Queue of recently visited items
    std::deque<HistoryEntry> m_recentItems;

    /// Prepared query for an individual HistoryEntry entry
    QSqlQuery *m_queryHistoryItem;

    /// Prepared query for a visit entry regarding a HistoryEntry
    QSqlQuery *m_queryVisit;

    /// Prepared query to fetch a history entry by its URL
    QSqlQuery *m_queryGetEntryByUrl;
};

#endif // HISTORYSTORE_H
