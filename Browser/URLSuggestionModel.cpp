#include "BrowserApplication.h"
#include "BookmarkManager.h"
#include "HistoryManager.h"
#include "URLSuggestionModel.h"

#include <QQueue>
#include <QDebug>

//TODO: Add method to insert new urls to the model as new urls are visited by the user

URLSuggestionModel::URLSuggestionModel(QObject *parent) :
    QAbstractListModel(parent),
    m_urls()
{
    loadURLs();
}

int URLSuggestionModel::rowCount(const QModelIndex &/*parent*/) const
{
    return m_urls.size();
}

QVariant URLSuggestionModel::data(const QModelIndex &index, int role) const
{
    if (index.column() == 0 && (role == Qt::EditRole || role == Qt::DisplayRole))
        return m_urls.at(index.row());

    return QVariant();
}

Qt::ItemFlags URLSuggestionModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return 0;
    return Qt::ItemIsEditable | Qt::ItemIsSelectable | QAbstractListModel::flags(index);
}

void URLSuggestionModel::loadURLs()
{
    QSet<QString> foundURLs = loadBookmarkURLs().unite(loadHistoryURLs());
    for (const QString &url : foundURLs)
        m_urls.append(url);
}

QSet<QString> URLSuggestionModel::loadBookmarkURLs()
{
    QSet<QString> urls;

    std::shared_ptr<BookmarkManager> bookmarkMgr = sBrowserApplication->getBookmarkManager();
    if (!bookmarkMgr.get())
        return urls;

    BookmarkFolder *f = nullptr;
    QQueue<BookmarkFolder*> folders;
    folders.enqueue(bookmarkMgr->getRoot());
    while (!folders.empty())
    {
        f = folders.dequeue();
        for (BookmarkFolder *folder : f->folders)
            folders.enqueue(folder);

        for (Bookmark *b : f->bookmarks)
            urls.insert(b->URL);
    }

    return urls;
}

QSet<QString> URLSuggestionModel::loadHistoryURLs()
{
    QSet<QString> urls;

    auto urlList = sBrowserApplication->getHistoryManager()->getVisitedURLs();
    for (auto url : urlList)
        urls.insert(url);
    return urls;
}
