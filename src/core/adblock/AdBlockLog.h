#ifndef ADBLOCKLOG_H
#define ADBLOCKLOG_H

#include "AdBlockFilter.h"

#include <QDateTime>
#include <QHash>
#include <QUrl>

#include <vector>

namespace adblock
{

/// The types of actions that can be done to a network request by an ad block filter
enum class FilterAction
{
    Allow,
    Block,
    Redirect
};

/**
 * @struct LogEntry
 * @brief Contains information about a network request that was affected by an \ref AdBlockFilter
 * @ingroup AdBlock
 */
struct LogEntry
{
    /// The action that was done to the request
    FilterAction Action;

    /// The source from which the request was made
    QUrl FirstPartyUrl;

    /// The resource that was requested
    QUrl RequestUrl;

    /// The type or types associated with the requested resource
    ElementType ResourceType;

    /// The filter rule that was applied to the request
    QString Rule;

    /// The time of the log entry
    QDateTime Timestamp;
};

/**
 * @class AdBlockLog
 * @brief This class stores information about any recent network requests that were affected by
 *        an \ref AdBlockFilter
 * @ingroup AdBlock
 */
class AdBlockLog : public QObject
{
    Q_OBJECT

public:
    /// Constructs the log with a given parent
    AdBlockLog(QObject *parent = nullptr);

    /// Logging destructor
    ~AdBlockLog();

    /**
     * @brief addEntry Adds a network action performed by the ad block system to the logs
     * @param action The action that was done to the request
     * @param firstPartyUrl The source from which the request was made
     * @param requestUrl The resource that was requested
     * @param resourceType The type or types associated with the requested resource
     * @param rule The filter rule that was applied to the request
     * @param timestamp The time of the network action
     */
    void addEntry(FilterAction action, const QUrl &firstPartyUrl, const QUrl &requestUrl,
                  ElementType resourceType, const QString &rule, const QDateTime &timestamp);

    /// Returns all log entries
    std::vector<LogEntry> getAllEntries() const;

    /// Returns all log entries associated with the given first party request url, or
    /// an empty container if no entries are found
    const std::vector<LogEntry> &getEntriesFor(const QUrl &firstPartyUrl);

protected:
    /// Called on a regular interval to prune older log entries
    void timerEvent(QTimerEvent *event) override;

private Q_SLOTS:
    /// Removes any entries from the logs that are more than 30 minutes old
    void pruneLogs();

private:
    /// Hashmap of first party URLs associated with requests, to containers of their associated log entries
    QHash<QUrl, std::vector<LogEntry>> m_entries;

    /// Unique identifier of the log pruning timer
    int m_timerId;
};

}

#endif // ADBLOCKLOG_H
