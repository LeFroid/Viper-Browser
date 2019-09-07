#ifndef HISTORYSUGGESTOR_H
#define HISTORYSUGGESTOR_H

#include "IURLSuggestor.h"
#include "URLSuggestionListModel.h"

#include <map>
#include <mutex>
#include <vector>

class FaviconManager;
class HistoryManager;

/**
 * @class HistorySuggestor
 * @brief Handles URL suggestions that rely on the user's browsing history
 *        as a data source
 */
class HistorySuggestor final : public IURLSuggestor
{
public:
    /// Default constructor
    HistorySuggestor() = default;

    /// Injects the history manager and favicon manager dependencies
    void setServiceLocator(const ViperServiceLocator &serviceLocator) override;

    /// Suggests history entries to the user, based on their text input
    std::vector<URLSuggestion> getSuggestions(const std::atomic_bool &working,
                                              const QString &searchTerm,
                                              const QStringList &searchTermParts,
                                              const FastHashParameters &hashParams) override;

    /// Refreshes the history word database when invoked
    void timerEvent() override;

private:
    /// Loads the word database, and word-history entry mappings
    void loadHistoryWords();

private:
    /// Gathers icons which are sent in the suggestion results
    FaviconManager *m_faviconManager;

    /// Data source for the history-based suggestions
    HistoryManager *m_historyManager;

    /// History-related word dictionary
    std::map<int, QString> m_historyWords;

    /// Map of history entry IDs to the IDs of associated words. Used to make educated suggestions based on user input
    std::map<int, std::vector<int>> m_historyWordMap;

    /// Used for access control to the history word maps
    std::mutex m_mutex;
};

#endif // HISTORYSUGGESTOR_H
