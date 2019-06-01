#ifndef URLSUGGESTIONWORKER_H
#define URLSUGGESTIONWORKER_H

#include "ServiceLocator.h"
#include "URLSuggestion.h"
#include "URLSuggestionListModel.h"

#include <atomic>
#include <string>
#include <vector>

#include <QFuture>
#include <QFutureWatcher>
#include <QObject>
#include <QString>
#include <QStringList>

class BookmarkManager;
class FaviconManager;
class HistoryManager;

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

signals:
    /// Emitted when a suggestion search is finished, passing a reference to each URL matching the input pattern
    void finishedSearch(const std::vector<URLSuggestion> &results);

private:
    /// The suggestion search operation working in a separate thread
    void searchForHits();

    /// Checks if an item with the given page title, url and optionally shortcut matches the search term, returning
    /// the corresponding type after evaluating all criteria. Returns MatchType::None when there is no match
    MatchType getMatchType(const QString &title, const QString &url, const QString &shortcut = QString());

    /// Check if, for a very small search term (< 5 chars), either a part of the page title or URL begins with
    /// the characters in the search term
    MatchType getMatchTypeForSmallSearchTerm(const QString &title, const QString &url);

    /// Checks if an item with the given page title, url and optionally shortcut matches the search term, returning
    /// true on a match and false if not matching
    bool isEntryMatch(const QString &title, const QString &url, const QString &shortcut = QString());

    /// Check if, for a very small search term (< 5 chars), either a part of the page title or URL begins with
    /// the characters in the search term.
    bool isMatchForSmallSearchTerm(const QString &title, const QString &url);

    /// Applies the Rabin-Karp string matching algorithm to determine whether or not the haystack contains the search term
    bool isStringMatch(const QString &haystack);

    /// Generates a hash of the search term before looking for suggestions
    void hashSearchTerm();

private:
    /// True if the worker thread is active, false if else
    std::atomic_bool m_working;

    /// The search term used to find suggestions
    QString m_searchTerm;

    /// The search term, split by the ' ' character for partial string matching
    QStringList m_searchWords;

    /// True if the search term contains a scheme (used for string matching)
    bool m_searchTermHasScheme;

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

    /// Pointer to the bookmark manager, used for suggeseting urls
    BookmarkManager *m_bookmarkManager;

    /// Gathers icons which are sent in the suggestion results
    FaviconManager *m_faviconManager;

    /// Used to determine history-based matches
    HistoryManager *m_historyManager;
};

#endif // URLSUGGESTIONWORKER_H
