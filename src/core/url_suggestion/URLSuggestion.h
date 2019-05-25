#ifndef URLSUGGESTION_H
#define URLSUGGESTION_H

#include <QDateTime>
#include <QIcon>
#include <QString>

class BookmarkNode;
struct HistoryEntry;
class URLRecord;

/**
 * @struct URLSuggestion
 * @brief Container for the information contained in a single row of data in the \ref URLSuggestionListModel
 */
struct URLSuggestion
{
    /// Default constructor
    URLSuggestion() = default;

    /// Constructs the URL suggestion given a bookmark node and its corresponding history entry
    URLSuggestion(const BookmarkNode *bookmark, const HistoryEntry &historyEntry);

    /// Constructs the URL suggestion from a history record and an icon
    URLSuggestion(const URLRecord &record, const QIcon &icon);

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

    /// Flag indicating whether or not the host of this url starts with the user input string
    bool IsHostMatch;

    /// Flag indicating whether or not this url is bookmarked
    bool IsBookmark;
};

#endif // URLSUGGESTION_H
