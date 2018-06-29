#include "URLSuggestionListModel.h"

URLSuggestion::URLSuggestion(const QIcon &icon, const QString &title, const QString &url) :
    Favicon(icon),
    Title(title),
    URL(url)
{
}

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

    return QVariant();
}

void URLSuggestionListModel::setSuggestions(std::vector<URLSuggestion> suggestions)
{
    beginResetModel();
    m_suggestions = std::move(suggestions);
    endResetModel();
}
