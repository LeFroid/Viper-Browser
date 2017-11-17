#include "BookmarkFolderModel.h"
#include "BookmarkNode.h"
#include <cstdint>

BookmarkFolderModel::BookmarkFolderModel(std::shared_ptr<BookmarkManager> bookmarkMgr, QObject *parent) :
    QAbstractItemModel(parent),
    m_root(bookmarkMgr->getRoot()),
    m_bookmarkMgr(bookmarkMgr)
{
}

BookmarkFolderModel::~BookmarkFolderModel()
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
        BookmarkNode *folder = getItem(index);
        folder->setName(value.toString());
        m_bookmarkMgr->updatedFolderName(folder);
        emit dataChanged(index, index);
        return true;
    }
    return false;
}

Qt::ItemFlags BookmarkFolderModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return Qt::NoItemFlags;

    //TODO: Allow drag and drop events, tie to an event handler to change the relative position or order of folders
    return Qt::ItemIsEditable | QAbstractItemModel::flags(index);
}

bool BookmarkFolderModel::insertRows(int row, int count, const QModelIndex &parent)
{
    BookmarkNode *parentFolder = getItem(parent);
    if (!parentFolder)
        return false;
    //TODO: implement ability to move position of folder to row + i
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
        m_bookmarkMgr->removeFolder(getItem(index(row + i, 0, parent)));
    endRemoveRows();
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

