#ifndef URLSUGGESTIONLISTMODEL_H
#define URLSUGGESTIONLISTMODEL_H

#include <vector>

#include <QAbstractListModel>
#include <QDateTime>
#include <QIcon>
#include <QString>

/**
 * @struct URLSuggestion
 * @brief Container for the information contained in a single row of data in the \ref URLSuggestionListModel
 */
struct URLSuggestion
{
    /// Icon associated with the url
    QIcon Favicon;

    /// Last known title of the page with this url
    QString Title;
    
    /// URL of the page, in string form
    QString URL;

    /// Date and time of the last visit to this url
    QDateTime LastVisit;
    
    /// Flag indicating whether or not this url is bookmarked
    bool IsBookmark;

    /// Number of visits to the page with this url
    int VisitCount;

    /// Flag indicating whether or not the host of this url starts with the user input string
    bool IsHostMatch;

    /// Number of times the URL was explicitly entered by the user
    int URLTypedCount;

    /// Default constructor
    URLSuggestion() = default;

    /// Constructs the url suggestion with all of the data it needs to store
    URLSuggestion(const QIcon &icon, const QString &title, const QString &url, 
            QDateTime lastVisit, bool isBookmark, int visitCount);
};

/**
 * @class URLSuggestionListModel
 * @brief Contains a list of URLs to be suggested to the user as they
 *        type into the \ref URLLineEdit
 */
class URLSuggestionListModel : public QAbstractListModel
{
    Q_OBJECT

public:
    enum Role
    {
        Favicon  = Qt::UserRole + 1,
        Title    = Qt::UserRole + 2,
        Link     = Qt::UserRole + 3,
        Bookmark = Qt::UserRole + 4
    };

public:
    /// Constructs the URL suggestion list model with the given parent
    explicit URLSuggestionListModel(QObject *parent = nullptr);

    /// Returns the number of rows under the given parent
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;

    /// Returns the data stored under the given role for the item referred to by the index.
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    /// Removes count rows starting with the given row under parent parent from the model.
    /// Returns true if the rows were successfully removed, otherwise returns false.
    bool removeRows(int row, int count, const QModelIndex &parent = QModelIndex()) override;

public slots:
    /// Sets the suggested items to be displayed in the model
    void setSuggestions(std::vector<URLSuggestion> suggestions);

private:
    /// Contains suggested URLs based on the current input
    std::vector<URLSuggestion> m_suggestions;
};

#endif // URLSUGGESTIONLISTMODEL_H
