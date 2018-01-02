#include "BrowserApplication.h"
#include "BookmarkManager.h"
#include "BookmarkNode.h"
#include "HistoryManager.h"
#include "URLSuggestionModel.h"

#include <algorithm>
#include <QQueue>

const static int URL_Counter_Threshold = 15;

URLSuggestionModel::URLSuggestionModel(QObject *parent) :
    QAbstractListModel(parent),
    m_urls(),
    m_counter(0)
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

void URLSuggestionModel::onPageVisited(const QString &url, const QString &title)
{
    if (url.isEmpty() || title.isEmpty())
        return;

    m_urls.push_back(QString("%1 - %2").arg(url).arg(title));

    // Check counter threshold
    if (++m_counter >= URL_Counter_Threshold)
    {
        removeDuplicates();
        m_counter = 0;
    }
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

    BookmarkManager *bookmarkMgr = sBrowserApplication->getBookmarkManager();
    if (!bookmarkMgr)
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

void URLSuggestionModel::removeDuplicates()
{
    m_urls.erase(std::unique(m_urls.begin(), m_urls.end()), m_urls.end());
}

