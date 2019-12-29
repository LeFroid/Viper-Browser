#include "URLSuggestionListModel.h"

URLSuggestionListModel::URLSuggestionListModel(QObject *parent) :
    QAbstractListModel(parent),
    m_suggestions()
{
}

int URLSuggestionListModel::rowCount(const QModelIndex &/*parent*/) const
{
    return static_cast<int>(m_suggestions.size());
}

QVariant URLSuggestionListModel::data(const QModelIndex &index, int role) const
{
    if (index.row() < 0 || index.row() >= rowCount())
        return QVariant();

    const URLSuggestion &item = m_suggestions.at(index.row());
    if (role == Role::Favicon)
        return item.Favicon;
    else if (role == Role::Title)
        return item.Title;
    else if (role == Role::Link)
        return item.URL;
    else if (role == Role::Bookmark)
        return item.IsBookmark;
    else if (role == Role::MatchedBy)
        return static_cast<int>(item.Type);

    return QVariant();
}

void URLSuggestionListModel::setSuggestions(const std::vector<URLSuggestion> &suggestions)
{
    beginResetModel();
    m_suggestions = suggestions;
    endResetModel();
}

bool URLSuggestionListModel::removeRows(int row, int count, const QModelIndex &parent)
{
    if (row < 0 || row + count > rowCount())
        return false;

    beginRemoveRows(parent, row, row + count - 1);
    m_suggestions.erase(m_suggestions.begin() + row, m_suggestions.begin() + row + count);
    endRemoveRows();

    return true;
}
