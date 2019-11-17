#ifndef HISTORYMANAGER_H
#define HISTORYMANAGER_H

#include "CommonUtil.h"
#include "ClearHistoryOptions.h"
#include "DatabaseTaskScheduler.h"
#include "FavoritePagesManager.h"
#include "ServiceLocator.h"
#include "ISettingsObserver.h"
#include "URLRecord.h"

#include <QDateTime>
#include <QHash>
#include <QIcon>
#include <QList>
#include <QMetaType>
#include <QUrl>

#include <deque>
#include <unordered_map>
#include <vector>

class HistoryStore;

/// Available policies for storage of browsing history data
enum class HistoryStoragePolicy
{
    Remember    = 0,
    SessionOnly = 1,
    Never       = 2
};

Q_DECLARE_METATYPE(HistoryStoragePolicy)

/**
 * @class HistoryManager
 * @brief Maintains the state of the browsing history that belongs to a user profile
 */
class HistoryManager : public QObject, public ISettingsObserver
{
    friend class DatabaseFactory;

    Q_OBJECT

public:
    using const_iterator = std::unordered_map<QString, URLRecord>::const_iterator;

    /// Constructs the history manager, given the path to the history database
    explicit HistoryManager(const ViperServiceLocator &serviceLocator, DatabaseTaskScheduler &taskScheduler);

    /// Destructor
    ~HistoryManager();

    /// Returns a const_iterator to the first element in the history hash map
    const_iterator begin() const { return m_historyItems.cbegin(); }

    /// Returns a const_iterator to the end of the history hash map
    const_iterator end() const { return m_historyItems.cend(); }

    /// Clears all browsing history
    void clearAllHistory();

    /// Clears history stored from the given start time to the present
    void clearHistoryFrom(const QDateTime &start);

    /// Clears history within the given {start,end} date-time pair
    void clearHistoryInRange(std::pair<QDateTime, QDateTime> range);

    /// Adds an entry to the history data store, given the URL, page title, time of visit, and the requested URL
    void addVisit(const QUrl &url, const QString &title, const QDateTime &visitTime, const QUrl &requestedUrl, bool wasTypedByUser);

    /// Loads a list of all \ref URLRecord visited between the given start date and end dates, passing them on to
    /// the callback once the data has been fetched
    void getHistoryBetween(const QDateTime &startDate, const QDateTime &endDate, std::function<void(std::vector<URLRecord>)> callback);

    /// Loads a list of all \ref URLRecord visited from the given start date to the present, passing them on to
    /// the callback once the data has been fetched
    void getHistoryFrom(const QDateTime &startDate, std::function<void(std::vector<URLRecord>)> callback);

    /// Checks if the given URL is contained in the history database, passing the result as a boolean
    /// in the given callback function
    void contains(const QUrl &url, std::function<void(bool)> callback);

    /// Returns a history record corresponding to the given URL, or an empty record if it was not found in the
    /// database
    HistoryEntry getEntry(const QUrl &url) const;

    /// Returns a queue of recently visited items, with the most recent visits being at the front of the queue
    const std::deque<HistoryEntry> &getRecentItems() const { return m_recentItems; }

    /// Fetches the number of times the host was visited, passing it to the given callback when the
    /// result has been retrieved
    void getTimesVisitedHost(const QUrl &host, std::function<void(int)> callback);

    /// Returns the history manager's storage policy
    HistoryStoragePolicy getStoragePolicy() const;

    /// Sets the policy to be followed for storing browsing history
    void setStoragePolicy(HistoryStoragePolicy policy);

    /// Fetches the set of most frequently visited web pages, up to the given limit. This is used to
    /// determine which web pages' thumbnails to retrieve for the "New Tab" page
    void loadMostVisitedEntries(int limit, std::function<void(std::vector<WebPageInformation>)> callback);

    /// Loads the word table into a map, returning the data in the callback function. Used by the
    /// URL suggestion worker when recommending matches based on user input
    void loadWordDatabase(std::function<void(std::map<int, QString>)> callback);

    /// Loads a mapping of history entries to the lists of their corresponding words
    void loadHistoryWordMapping(std::function<void(std::map<int, std::vector<int>>)> callback);

Q_SIGNALS:
    /// Emitted when a page has been visited
    void pageVisited(const QUrl &url, const QString &title);

    /// Emitted when some or all of the history collection has been erased
    void historyCleared();

private Q_SLOTS:
    /// Listens for any settings changes that affect the history manager
    void onSettingChanged(BrowserSetting setting, const QVariant &value) override;

private:
    /// Adds the history visit to the in-memory history store.
    void addVisitToLocalStore(const QUrl &url, const QString &title, const QDateTime &visitTime, bool wasTypedByUser);

    /// Handles the recent history record load event - called during instantiation of the \ref HistoryStore
    void onRecentItemsLoaded(std::deque<HistoryEntry> &&entries);

    /// Handles the history record load event - called during instantiation of the \ref HistoryStore
    void onHistoryRecordsLoaded(std::vector<URLRecord> &&records);

private:
    /// Reference to the task scheduler. Needed to queue work for the \ref HistoryStore
    DatabaseTaskScheduler &m_taskScheduler;

    /// Hash map of history URL strings to their corresponding data structure
    std::unordered_map<QString, URLRecord> m_historyItems;

    /// Queue of recently visited items
    std::deque<HistoryEntry> m_recentItems;

    /// History storage policy
    HistoryStoragePolicy m_storagePolicy;

    /// Pointer to the history store
    HistoryStore *m_historyStore;

    /// Unique id of the most recent entry in the database
    uint64_t m_lastVisitId;
};

#endif // HISTORYMANAGER_H
