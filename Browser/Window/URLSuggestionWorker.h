#ifndef URLSUGGESTIONWORKER_H
#define URLSUGGESTIONWORKER_H

#include "URLSuggestionListModel.h"

#include <atomic>
#include <vector>

#include <QFuture>
#include <QFutureWatcher>
#include <QObject>
#include <QString>

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

signals:
    /// Emitted when a suggestion search is finished, passing a reference to each URL matching the input pattern
    void finishedSearch(const std::vector<URLSuggestion> &results);

private:
    /// The suggestion search operation working in a separate thread
    void searchForHits();

    /// Applies the Rabin-Karp string matching algorithm to determine whether or not the haystack contains the search term
    bool isStringMatch(const QString &haystack);

    /// Generates a hash of the search term before looking for suggestions
    void hashSearchTerm();

    /// Computes and returns base^exp
    inline quint64 quPow(quint64 base, quint64 exp) const
    {
        quint64 result = 1;
        while (exp)
        {
            if (exp & 1)
                result *= base;
            exp >>= 1;
            base *= base;
        }
        return result;
    }

private:
    /// True if the worker thread is active, false if else
    std::atomic_bool m_working;

    /// The search term used to find suggestions
    QString m_searchTerm;

    /// Future of the searchForHits operation
    QFuture<void> m_suggestionFuture;

    /// Watches the searchForHits future, emits finishedSearch when the search operation is complete
    QFutureWatcher<void> *m_suggestionWatcher;

    /// Stores the suggested URLs based on the current input
    std::vector<URLSuggestion> m_suggestions;

    /// Used for string hash comparisons in implementation of Rabin-Karp algorithm
    quint64 m_differenceHash;

    /// Contains a hash of the search term string
    size_t m_searchTermHash;
};

#endif // URLSUGGESTIONWORKER_H
