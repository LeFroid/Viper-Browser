#include "BookmarkFolderModel.h"
#include "BookmarkNode.h"
#include "BookmarkManager.h"
#include <vector>
#include <cstdint>

#include <QByteArray>
#include <QDataStream>
#include <QIODevice>
#include <QMimeData>

BookmarkFolderModel::BookmarkFolderModel(BookmarkManager *bookmarkMgr, QObject *parent) :
    QAbstractItemModel(parent),
    m_root(bookmarkMgr->getRoot()),
    m_bookmarksBar(bookmarkMgr->getBookmarksBar()),
    m_bookmarkMgr(bookmarkMgr)
{
}

QModelIndex BookmarkFolderModel::index(int row, int column, const QModelIndex &parent) const
{
    if (parent.isValid() && column != 0)
        return QModelIndex();

    // Attempt to find child
    BookmarkNode *parentFolder = getItem(parent);
    BookmarkNode *child = nullptr;
    int numChildren = parentFolder->getNumChildren();
    if (row >= numChildren)
        return QModelIndex();

    int folderIdx = 0;
    for (int i = 0; i < numChildren; ++i)
    {
        BookmarkNode *n = parentFolder->getNode(i);
        if (n->getType() == BookmarkNode::Folder)
        {
            if (folderIdx == row)
            {
                child = n;
                break;
            }
            ++folderIdx;
        }
    }

    // Create index if found, otherwise return empty index
    if (child)
        return createIndex(row, column, child);

    return QModelIndex();
}

QModelIndex BookmarkFolderModel::parent(const QModelIndex &index) const
{
    if (!index.isValid())
        return QModelIndex();

    BookmarkNode *child = getItem(index);
    BookmarkNode *parent = child->getParent();
    if (parent == nullptr)
        return QModelIndex();

    int parentPos = 0;
    int numChildren = parent->getNumChildren();
    for (int i = 0; i < numChildren; ++i)
    {
        BookmarkNode *n = parent->getNode(i);
        if (n->getType() == BookmarkNode::Folder)
        {
            if (n == child)
                break;
            ++parentPos;
        }
    }
    return createIndex(parentPos, 0, parent);
}

int BookmarkFolderModel::rowCount(const QModelIndex &parent) const
{
    BookmarkNode *f = getItem(parent);
    if (!f)
        return 0;
    int rows = 0;
    int numChildren = f->getNumChildren();
    for (int i = 0; i < numChildren; ++i)
        if (f->getNode(i)->getType() == BookmarkNode::Folder)
            ++rows;
    return rows;
}

int BookmarkFolderModel::columnCount(const QModelIndex &/*parent*/) const
{
    // Fixed number of columns
    return 1;
}

QVariant BookmarkFolderModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    if (role == Qt::DisplayRole || role == Qt::EditRole)
    {
        BookmarkNode *folder = getItem(index);
        return folder->getName();
    }
    return QVariant();
}

bool BookmarkFolderModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (index.isValid() && role == Qt::EditRole)
    {
        if (BookmarkNode *folder = getItem(index))
        {
            m_bookmarkMgr->setBookmarkName(folder, value.toString());
            Q_EMIT dataChanged(index, index);
            return true;
        }
    }
    return false;
}

Qt::ItemFlags BookmarkFolderModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return Qt::NoItemFlags;

    Qt::ItemFlags itemFlags = Qt::ItemIsDropEnabled | QAbstractItemModel::flags(index);
    if (BookmarkNode *n = getItem(index))
    {
        if (n != m_bookmarksBar)
            itemFlags |= Qt::ItemIsEditable | Qt::ItemIsDragEnabled;
    }
    return itemFlags;
}

bool BookmarkFolderModel::insertRows(int row, int count, const QModelIndex &parent)
{
    BookmarkNode *parentFolder = getItem(parent);
    if (!parentFolder)
        return false;
    beginInsertRows(parent, row, row + count - 1);
    for (int i = 0; i < count; ++i)
        static_cast<void>(m_bookmarkMgr->addFolder(QString("New Folder %1").arg(i), parentFolder));
    endInsertRows();
    return true;
}

bool BookmarkFolderModel::removeRows(int row, int count, const QModelIndex &parent)
{
    beginRemoveRows(parent, row, row + count - 1);
    for (int i = 0; i < count; ++i)
        m_bookmarkMgr->removeBookmark(getItem(index(row + i, 0, parent)));
    endRemoveRows();
    return true;
}

QStringList BookmarkFolderModel::mimeTypes() const
{
    QStringList types;
    types << "application/x-bookmark-data";
    return types;
}

Qt::DropActions BookmarkFolderModel::supportedDropActions() const
{
    return Qt::MoveAction;
}

QMimeData *BookmarkFolderModel::mimeData(const QModelIndexList &indexes) const
{
    QMimeData *mimeData = new QMimeData();
    QByteArray encodedData;
    QDataStream stream(&encodedData, QIODevice::WriteOnly);
    for (const QModelIndex &index : indexes)
    {
        if (index.isValid())
        {
            if (BookmarkNode *folder = static_cast<BookmarkNode*>(index.internalPointer()))
                stream << folder;
            //stream << getItem(index);
        }
    }
    mimeData->setData("application/x-bookmark-data", encodedData);
    return mimeData;
}

bool BookmarkFolderModel::dropMimeData(const QMimeData *data, Qt::DropAction action, int /*row*/, int /*column*/, const QModelIndex &parent)
{
    if (action == Qt::IgnoreAction)
        return true;

    if (!data || action != Qt::MoveAction)
        return false;

    if (!data->hasFormat("application/x-bookmark-data"))
        return false;

    // If given parent model index is not valid, the item was dropped onto the root folder
    bool droppedInRoot = !parent.isValid();
    BookmarkNode *targetNode = getItem(parent);

    // Retrieve pointers to bookmark nodes from serialized data
    QByteArray encodedData = data->data("application/x-bookmark-data");
    QDataStream stream(&encodedData, QIODevice::ReadOnly);

    std::vector<BookmarkNode*> droppedNodes;
    while (!stream.atEnd())
    {
        BookmarkNode *node;
        stream >> node;
        droppedNodes.push_back(node);
    }

    // Handle each dropped node depending on its type
    //  - For folders, adjust their parent and/or position.
    //  - For bookmarks, if dropped onto root folder, ignore, otherwise change their parent folder
    Q_EMIT beginMovingBookmarks();
    beginResetModel();
    for (BookmarkNode *n : droppedNodes)
    {
        switch (n->getType())
        {
            case BookmarkNode::Folder:
            {
                BookmarkNode *newPtr = m_bookmarkMgr->setBookmarkParent(n, targetNode);
                Q_EMIT movedFolder(n, newPtr);
                break;
            }
            case BookmarkNode::Bookmark:
            {
                if (droppedInRoot)
                    break;
                m_bookmarkMgr->setBookmarkParent(n, targetNode);
                break;
            }
        }
    }
    endResetModel();
    Q_EMIT endMovingBookmarks();

    return true;
}

BookmarkNode *BookmarkFolderModel::getItem(const QModelIndex &index) const
{
    if (index.isValid())
    {
        if (BookmarkNode *folder = static_cast<BookmarkNode*>(index.internalPointer()))
            return folder;
    }
    return m_root;
}

