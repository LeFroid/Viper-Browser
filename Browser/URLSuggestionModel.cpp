#include "BrowserApplication.h"
#include "BookmarkManager.h"
#include "BookmarkNode.h"
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
        m_urls.push_back(url);
}

QSet<QString> URLSuggestionModel::loadBookmarkURLs()
{
    QSet<QString> urls;

    std::shared_ptr<BookmarkManager> bookmarkMgr = sBrowserApplication->getBookmarkManager();
    if (!bookmarkMgr.get())
        return urls;

    BookmarkNode *f = nullptr;
    QQueue<BookmarkNode*> folders;
    folders.enqueue(bookmarkMgr->getRoot());
    while (!folders.empty())
    {
        f = folders.dequeue();
        int numChildren = f->getNumChildren();
        for (int i = 0; i < numChildren; ++i)
        {
            BookmarkNode *n = f->getNode(i);
            if (n->getType() == BookmarkNode::Folder)
                folders.enqueue(n);
            else
                urls.insert(QString("%1 - %2").arg(n->getURL()).arg(n->getName()));
        }
    }

    return urls;
}

QSet<QString> URLSuggestionModel::loadHistoryURLs()
{
    QSet<QString> urls;

    HistoryManager *histMgr = sBrowserApplication->getHistoryManager();
    for (auto it = histMgr->getHistIterBegin(); it != histMgr->getHistIterEnd(); ++it)
    {
        urls.insert(QString("%1 - %2").arg(it.key()).arg(it->Title));
    }

    return urls;
}
