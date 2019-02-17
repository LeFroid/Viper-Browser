#ifndef ADBLOCKREQUESTHANDLER_H
#define ADBLOCKREQUESTHANDLER_H

#include "AdBlockFilter.h"
#include "AdBlockFilterContainer.h"
#include "AdBlockSubscription.h"

#include <deque>
#include <vector>

#include <QHash>
#include <QObject>
#include <QString>
#include <QWebEngineUrlRequestInfo>

class AdBlockLog;

/**
 * @class AdBlockRequestHandler
 * @brief Handles network requests, checking for any matching filters
 *        to allow a request, block it, or redirect to something else.
 * @ingroup AdBlock
 */
class AdBlockRequestHandler : public QObject
{
    friend class AdBlockManager;

    Q_OBJECT

public:
    /// Constructs the request handler with the given parent
    explicit AdBlockRequestHandler(AdBlockFilterContainer &filterContainer, AdBlockLog *log, QObject *parent);

    /// Returns the number of ads that were blocked on the page with the given URL during its last page load
    int getNumberAdsBlocked(const QUrl &url) const;

    /// Returns the total number of network requests that have been blocked by the ad blocking system
    quint64 getTotalNumberOfBlockedRequests() const;

    /// Returns true if the given request should be blocked, false if else
    bool shouldBlockRequest(QWebEngineUrlRequestInfo &info);

protected:
    /// Sets the counter that stores the total number of network requests that have been blocked
    void setTotalNumberOfBlockedRequests(quint64 count);

    /// Begins to keep track of the number of ads that were blocked on the page with the given url
    void loadStarted(const QUrl &url);

private:
    /// Returns the \ref ElementType of the network request, which is used to check for filter option/type matches
    ElementType getRequestType(const QWebEngineUrlRequestInfo &info) const;

private:
    /// Reference to the filter container
    AdBlockFilterContainer &m_filterContainer;

    /// Logging instance
    AdBlockLog *m_log;

    /// Stores the number of network requests that have been blocked by the ad block system
    quint64 m_numRequestsBlocked;

    /// Hash map of URLs to the number of requests that were blocked on that given URL
    QHash<QUrl, int> m_pageAdBlockCount;
};

#endif //ADBLOCKREQUESTHANDLER_H
