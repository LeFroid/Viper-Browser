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

    /// Sets a reference to the service locator, which is used to gather the dependencies required by this worker
    /// (namely, the \ref HistoryManager , \ref BookmarkManager , and \ref FaviconStore )
    void setServiceLocator(const ViperServiceLocator &serviceLocator);

    /// Sets the internal "is working" flag to false, in order to prevent unnecessary suggestion determinations
    void stopWork();

public Q_SLOTS:
    /// Begins a new search operation for suggestions related to the given string.
    /// Cancels any operations in progress at the time this method is called
    void findSuggestionsFor(const QString &text);

Q_SIGNALS:
    /// Emitted when a suggestion search is finished, passing a reference to each URL matching the input pattern
    void finishedSearch(const std::vector<URLSuggestion> &results);

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
