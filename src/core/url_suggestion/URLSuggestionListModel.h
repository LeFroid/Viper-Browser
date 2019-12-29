#ifndef URLSUGGESTIONLISTMODEL_H
#define URLSUGGESTIONLISTMODEL_H

#include "URLSuggestion.h"

#include <vector>

#include <QAbstractListModel>
#include <QDateTime>
#include <QIcon>
#include <QString>

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
        Favicon   = Qt::UserRole + 1,
        Title     = Qt::UserRole + 2,
        Link      = Qt::UserRole + 3,
        Bookmark  = Qt::UserRole + 4,
        MatchedBy = Qt::UserRole + 5
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

public Q_SLOTS:
    /// Sets the suggested items to be displayed in the model
    void setSuggestions(const std::vector<URLSuggestion> &suggestions);

private:
    /// Contains suggested URLs based on the current input
    std::vector<URLSuggestion> m_suggestions;
};

#endif // URLSUGGESTIONLISTMODEL_H
