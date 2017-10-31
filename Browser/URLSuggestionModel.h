#ifndef URLSUGGESTIONMODEL_H
#define URLSUGGESTIONMODEL_H

#include <QAbstractListModel>
#include <QList>
#include <QSet>
#include <QString>

/**
 * @class URLSuggestionModel
 * @brief Loads URLs visited by the user in their saved browsing history and bookmarks,
 *        which is then used to suggest a full URL for the user once they begin to type
 *        a URL into the location bar
 */
class URLSuggestionModel : public QAbstractListModel
{
    Q_OBJECT

public:
    explicit URLSuggestionModel(QObject *parent = nullptr);

    /// Returns the number of rows of data in the model (i.e. the number of URLs stored)
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;

    /// Returns the data at the given index and with the specified role
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    Qt::ItemFlags flags(const QModelIndex &index) const override;

private:
    /// Loads the URLs stored in the user's bookmarks and browsing history
    void loadURLs();

    /// Loads URLs stored in the bookmark database, returning a set of all found URLS
    QSet<QString> loadBookmarkURLs();

    /// Loads URLs stored in the browser history, returning a set of all found URLS
    QSet<QString> loadHistoryURLs();

private:
    /// List of saved URL strings
    QList<QString> m_urls;
};

#endif // URLSUGGESTIONMODEL_H
