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
#include <map>
#include <vector>

/**
 * @class HistoryStore
 * @brief Maintains the state of the browsing history that belongs to a user profile
 */
class HistoryStore : public DatabaseWorker
{
    friend class DatabaseFactory;

    enum class Statement
    {
        CreateHistoryRecord,  /// INSERT OR REPLACE INTO History(VisitID, URL, Title, URLTypedCount) VALUES(?, ?, ?, ?)
        UpdateHistoryRecord,  /// UPDATE History SET Title = ?, URLTypedCount = ? WHERE VisitID = ?
        CreateVisitRecord,    /// INSERT INTO Visits(VisitID, Date) VALUES (?, ?)
        CreateWordRecord,     /// INSERT OR IGNORE INTO Words(Word) VALUES(?)
        CreateUrlWordRecord,  /// INSERT OR IGNORE INTO URLWords(HistoryID, WordID) VALUES(?, (SELECT WordID FROM Words WHERE Word = ?))
        GetHistoryRecord      /// SELECT History.VisitID, History.URL, History.Title, History.URLTypedCount, V.NumVisits, ...
    };

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

    /// Returns the visits associated with a given \ref HistoryEntry
    std::vector<VisitEntry> getVisits(const HistoryEntry &record);

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

    /// Returns all of the words stored in the words table
    std::map<int, QString> getWords() const;

    /// Returns a mapping of history entries to the list of word IDs associated with them
    std::map<int, std::vector<int>> getEntryWordMapping() const;

    /// Fetches the set of most frequently visited web pages, up to the given limit. This is used to
    /// determine which web pages' thumbnails to retrieve for the "New Tab" page
    std::vector<WebPageInformation> loadMostVisitedEntries(int limit = 10);

    /// Adds an entry to the history data store, given the URL, page title, time of visit, and the requested URL
    void addVisit(const QUrl &url, const QString &title, const QDateTime &visitTime, const QUrl &requestedUrl, bool wasTypedByUser);

    /// Returns the last unique id of an entry in the visit database. This is an auto-incrementing value
    uint64_t getLastVisitId() const;

protected:
    /// Returns true if the history database contains the table structures needed for it to function properly,
    /// false if else.
    bool hasProperStructure() override;

    /// Creates the browsing history-related tables in the database
    void setup() override;

    /// Loads browsing history from the database
    void load() override;

private:
    /// Splits the given URL into distinct words, saving the association in the database
    void tokenizeAndSaveUrl(int visitId, const QUrl &url, const QString &title);

    /// Called during the load() routine, this checks if any of the table structures need to be updated
    void checkForUpdate();

    /// Removes history items that are more than six months old
    void purgeOldEntries();

private:
    /// Stores the last visit ID that has been used to record browsing history. Auto increments for each new history item
    uint64_t m_lastVisitID;

    /// Cache of prepared statements
    std::map<Statement, sqlite::PreparedStatement> m_statements;
};

#endif // HISTORYSTORE_H
