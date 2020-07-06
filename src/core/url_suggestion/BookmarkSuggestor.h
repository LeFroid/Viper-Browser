#ifndef BOOKMARKSUGGESTOR_H
#define BOOKMARKSUGGESTOR_H

#include "IURLSuggestor.h"
#include "URLSuggestionListModel.h"

class BookmarkManager;
class HistoryManager;

/**
 * @class BookmarkSuggestor
 * @brief Handles URL suggestions that rely on the user's bookmark
 *        collection as a data source.
 */
class BookmarkSuggestor final : public IURLSuggestor
{
public:
    /// Default constructor
    BookmarkSuggestor() = default;

    /// Injects the bookmark manager dependency, which is needed to make any suggestions for the user
    void setServiceLocator(const ViperServiceLocator &serviceLocator) override;

    /// Suggests bookmarks to the user, based on the given input
    std::vector<URLSuggestion> getSuggestions(const std::atomic_bool &working,
                                              const QString &searchTerm,
                                              const QStringList &searchTermParts,
                                              const FastHashParameters &hashParams) override;

private:
    /// Checks if an item with the given page title, url and optionally shortcut matches the search term, returning
    /// the corresponding type after evaluating all criteria. Returns MatchType::None when there is no match
    MatchType getMatchType(const QString &searchTerm,
                           const QStringList &searchTermParts,
                           const FastHashParameters &hashParams,
                           const QString &title,
                           const QString &url,
                           const QString &shortcut = QString());

    /// Check if, for a very small search term (< 5 chars), either a part of the page title or URL begins with
    /// the characters in the search term
    MatchType getMatchTypeForSmallSearchTerm(const QString &searchTerm, const QString &title, const QString &url);

private:
    /// Used to compare bookmarks to any search term
    BookmarkManager *m_bookmarkManager;

    /// Used to fetch metadata about bookmark URL entries
    HistoryManager *m_historyManager;

    /// String representing the location of the bookmark database
    //QString m_databaseFile;
};

#endif // BOOKMARKSUGGESTOR_H
