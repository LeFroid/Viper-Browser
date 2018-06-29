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

    void findSuggestionsFor(const QString &text);

signals:
    void finishedSearch(const std::vector<URLSuggestion> &results);

private:
    void searchForHits();

/*
signals:

public slots:
*/

private:
    std::atomic_bool m_working;

    QString m_searchTerm;

    QFuture<void> m_suggestionFuture;

    QFutureWatcher<void> m_suggestionWatcher;

    std::vector<URLSuggestion> m_suggestions;
};

#endif // URLSUGGESTIONWORKER_H
