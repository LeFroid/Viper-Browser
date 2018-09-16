#ifndef HISTORYMANAGER_H
#define HISTORYMANAGER_H

#include "ClearHistoryOptions.h"
#include "DatabaseWorker.h"

#include <QDateTime>
#include <QHash>
#include <QIcon>
#include <QList>
#include <QMetaType>
#include <QUrl>

#include <deque>
#include <mutex>
#include <vector>

/// Available policies for storage of browsing history data
enum class HistoryStoragePolicy
{
    Remember    = 0,
    SessionOnly = 1,
    Never       = 2
};

Q_DECLARE_METATYPE(HistoryStoragePolicy)

/**
 * @struct WebHistoryItem
 * @brief Contains data about a specific web URL visited by the user
 */
struct WebHistoryItem
{
    /// URL of the item
    QUrl URL;

    /// Title of the web page
    QString Title;

    /// Unique visit ID of the history item
    int VisitID;

    /// List of recent visits to this history item
    QList<QDateTime> Visits;

    /// Returns true if the two WebHistoryItem objects are the same, false if else
    bool operator ==(const WebHistoryItem &other) const
    {
        return (this->URL.toString().compare(other.URL.toString(), Qt::CaseSensitive) == 0);
    }
};

/**
 * @class HistoryManager
 * @brief Implements the QWebHistory interface to save the user's browsing
 *        history
 */
class HistoryManager : public QObject, private DatabaseWorker
{
    friend class DatabaseFactory;

    Q_OBJECT

public:
    /// Constructs the history manager, given the history database file path and a parent pointer
    explicit HistoryManager(const QString &databaseFile, QObject *parent = nullptr);

    /// Saves browsing history at exit
    virtual ~HistoryManager();

    /// Adds an entry to the browser's history
    void addHistoryEntry(const QString &url, const QString &title);

    /// Clears all browsing history
    void clearAllHistory();

    /// Clears history stored from the given start time to the present
    void clearHistoryFrom(const QDateTime &start);

    /// Clears history within the given {start,end} date-time pair
    void clearHistoryInRange(std::pair<QDateTime, QDateTime> range);

    /// Returns true if the history contains the given url, false if else. Will return
    /// false if private browsing mode is enabled
    bool historyContains(const QString &url) const;

    /// Returns a queue of recently visited items, with the most recent visits being at the front of the queue
    const std::deque<WebHistoryItem> &getRecentItems() const { return m_recentItems; }

    /// Returns a const_iterator to the first element in the history hash map
    QHash<QString, WebHistoryItem>::const_iterator getHistIterBegin() const { return m_historyItems.cbegin(); }

    /// Returns a const_iterator to the end of the history hash map
    QHash<QString, WebHistoryItem>::const_iterator getHistIterEnd() const { return m_historyItems.cend(); }

    /// Returns a list of all visited URLs
    QList<QString> getVisitedURLs() const { return m_historyItems.keys(); }

    /// Loads and returns a list of all \ref WebHistoryItem items visited from the given start date to the present
    std::vector<WebHistoryItem> getHistoryFrom(const QDateTime &startDate) const;

    /// Loads and returns a list of all \ref WebHistoryItem items visited between the given start date and end dates
    std::vector<WebHistoryItem> getHistoryBetween(const QDateTime &startDate, const QDateTime &endDate) const;

    /// Returns the number of times the user has visited the given website by its hostname
    int getTimesVisited(const QString &host);

    /// Returns the history manager's storage policy
    HistoryStoragePolicy getStoragePolicy() const;

    /// Sets the policy to be followed for storing browsing history
    void setStoragePolicy(HistoryStoragePolicy policy);

signals:
    /// Emitted when a page has been visited
    void pageVisited(const QString &url, const QString &title);

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

    /// Saves the record of the user visiting the history item at the given date-time.
    void saveVisit(const WebHistoryItem &item, const QDateTime &visitTime);

private:
    /// Stores the last visit ID that has been used to record browsing history. Auto increments for each new history item
    uint64_t m_lastVisitID;

    /// Hash map of history URL strings to their corresponding data structure
    QHash<QString, WebHistoryItem> m_historyItems;

    /// Queue of recently visited items
    std::deque<WebHistoryItem> m_recentItems;

    /// Prepared query for an individual WebHistoryItem entry
    QSqlQuery *m_queryHistoryItem;

    /// Prepared query for a visit entry regarding a WebHistoryItem
    QSqlQuery *m_queryVisit;

    /// History storage policy
    HistoryStoragePolicy m_storagePolicy;

    /// Mutex
    mutable std::mutex m_mutex;
};

#endif // HISTORYMANAGER_H
