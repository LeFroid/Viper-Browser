#ifndef HISTORYSUGGESTOR_H
#define HISTORYSUGGESTOR_H

#include "IURLSuggestor.h"
#include "URLSuggestionListModel.h"

#include <map>
#include <vector>

class BookmarkManager;
class FaviconManager;

namespace sqlite
{
    class Database;
    class PreparedStatement;
}

/**
 * @class HistorySuggestor
 * @brief Handles URL suggestions that rely on the user's browsing history
 *        as a data source
 */
class HistorySuggestor final : public IURLSuggestor
{
    /// Prepared statement types
    enum class Statement
    {
        SearchByWholeInput,
        SearchBySingleWord
    };

public:
    /// Default constructor
    HistorySuggestor() = default;

    /// Default destructor
    ~HistorySuggestor() = default;

    /// Injects the history manager and favicon manager dependencies
    void setServiceLocator(const ViperServiceLocator &serviceLocator) override;

    /// Specifies which history database file the suggestor should use. If not set, the
    /// suggestor will use the application settings to get the value
    void setHistoryFile(const QString &historyDbFile);

    /// Suggests history entries to the user, based on their text input
    std::vector<URLSuggestion> getSuggestions(const std::atomic_bool &working,
                                              const QString &searchTerm,
                                              const QStringList &searchTermParts,
                                              const FastHashParameters &hashParams) override;

private:
    /// Returns a list of URL suggestions based on the result of a history suggestion query
    std::vector<URLSuggestion> getSuggestionsFromQuery(const std::atomic_bool &working,
                                                       const QString &searchTerm,
                                                       MatchType queryMatchType,
                                                       sqlite::PreparedStatement &query);

    /// Connects to the history database and creates the prepared statement cache
    void setupConnection();

private:
    /// Determines whether or not a suggestion is also a bookmark
    BookmarkManager *m_bookmarkManager;

    /// Gathers icons which are sent in the suggestion results
    FaviconManager *m_faviconManager;

    /// History database handle
    std::unique_ptr<sqlite::Database> m_historyDb;

    /// Stores the location of the history database
    QString m_historyDatabaseFile;

    /// Prepared statements used by the suggestor
    std::map<Statement, sqlite::PreparedStatement> m_statements;
};

#endif // HISTORYSUGGESTOR_H
