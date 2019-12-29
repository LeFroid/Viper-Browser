#ifndef URLSUGGESTION_H
#define URLSUGGESTION_H

#include <QDateTime>
#include <QIcon>
#include <QMetaType>
#include <QString>

class BookmarkNode;
struct HistoryEntry;
class URLRecord;

/**
 * @enum MatchType
 * @brief A list of the ways in which a search term could lead to a given suggestion
 */
enum class MatchType : int
{
    /// No match
    None = 0,
    /// The web page's shortcut matches the search term
    Shortcut = 1,
    /// The page title matches the search term
    Title = 2,
    /// Match is from one or more of the individual words in the search term
    SearchWords = 3,
    /// The URL matches the search term
    URL = 4
};

/**
 * @struct URLSuggestion
 * @brief Container for the information contained in a single row of data in the \ref URLSuggestionListModel
 */
struct URLSuggestion
{
    /// Default constructor
    URLSuggestion() = default;

    /// Constructs the URL suggestion given a bookmark node, its corresponding history entry and the type of search term match
    URLSuggestion(const BookmarkNode *bookmark, const HistoryEntry &historyEntry, MatchType matchType);

    /// Constructs the URL suggestion from a history record, an icon and the type of search term match
    URLSuggestion(const URLRecord &record, const QIcon &icon, MatchType matchType);

    /// Icon associated with the url
    QIcon Favicon;

    /// Last known title of the page with this url
    QString Title;

    /// URL of the page, in string form
    QString URL;

    /// Date and time of the last visit to this url
    QDateTime LastVisit;

    /// Number of times the URL was explicitly entered by the user
    int URLTypedCount;

    /// Number of visits to the page with this url
    int VisitCount;

    /// Percent match (0-100), applicable only for match type "SearchWords"
    int PercentMatch;

    /// Flag indicating whether or not the host of this url starts with the user input string
    bool IsHostMatch;

    /// Flag indicating whether or not this url is bookmarked
    bool IsBookmark;

    /// Type of matching to the search term
    MatchType Type;

    /// History database identifier
    int HistoryId;
};

Q_DECLARE_METATYPE(URLSuggestion)

#endif // URLSUGGESTION_H
