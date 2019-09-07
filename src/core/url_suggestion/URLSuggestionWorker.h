#ifndef URLSUGGESTIONWORKER_H
#define URLSUGGESTIONWORKER_H

#include "IURLSuggestor.h"
#include "ServiceLocator.h"
#include "URLSuggestion.h"
#include "URLSuggestionListModel.h"

#include <atomic>
#include <memory>
#include <string>
#include <vector>

#include <QFuture>
#include <QFutureWatcher>
#include <QObject>
#include <QString>
#include <QStringList>

/**
 * @class URLSuggestionWorker
 * @brief Fetches URL suggestions to populate into the \ref URLSuggestionWidget as the
 *        user types a string of text into the \ref URLLineEdit widget
 */
class URLSuggestionWorker : public QObject
{
    Q_OBJECT
public:
    /// Constructs the URL suggestion worker
    explicit URLSuggestionWorker(QObject *parent = nullptr);

    /// Begins a new search operation for suggestions related to the given string.
    /// Cancels any operations in progress at the time this method is called
    void findSuggestionsFor(const QString &text);

    /// Sets a reference to the service locator, which is used to gather the dependencies required by this worker
    /// (namely, the \ref HistoryManager , \ref BookmarkManager , and \ref FaviconStore )
    void setServiceLocator(const ViperServiceLocator &serviceLocator);

Q_SIGNALS:
    /// Emitted when a suggestion search is finished, passing a reference to each URL matching the input pattern
    void finishedSearch(const std::vector<URLSuggestion> &results);

private Q_SLOTS:
    /// Called when the data refresh timer has completed a cycle
    void onHandlerTimerTick();

private:
    /// The suggestion search operation working in a separate thread
    void searchForHits();

    /// Generates a hash of the search term before looking for suggestions
    void hashSearchTerm();

private:
    /// True if the worker thread is active, false if else
    std::atomic_bool m_working;

    /// The search term used to find suggestions
    QString m_searchTerm;

    /// The search term, split by the ' ' character for partial string matching
    QStringList m_searchWords;

    /// Future of the searchForHits operation
    QFuture<void> m_suggestionFuture;

    /// Watches the searchForHits future, emits finishedSearch when the search operation is complete
    QFutureWatcher<void> *m_suggestionWatcher;

    /// Stores the suggested URLs based on the current input
    std::vector<URLSuggestion> m_suggestions;

    /// Wide-string equivalent to m_searchTerm
    std::wstring m_searchTermWideStr;

    /// Used for string hash comparisons in implementation of Rabin-Karp algorithm
    quint64 m_differenceHash;

    /// Contains a hash of the search term string
    quint64 m_searchTermHash;

    /// URL suggestion implementations
    std::vector<std::unique_ptr<IURLSuggestor>> m_handlers;
};

#endif // URLSUGGESTIONWORKER_H
