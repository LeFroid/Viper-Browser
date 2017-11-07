#include "BookmarkFolderModel.h"
#include <cstdint>

BookmarkFolderModel::BookmarkFolderModel(std::shared_ptr<BookmarkManager> bookmarkMgr, QObject *parent) :
    QAbstractItemModel(parent),
    m_root(new BookmarkFolder),
    m_bookmarkMgr(bookmarkMgr)
{
    // Create a false root for proper tree display
    m_root->id = -1;
    m_root->parent = nullptr;
    m_root->folders.push_back(m_bookmarkMgr->getRoot());

    m_bookmarkMgr->m_root.parent = m_root;
}

BookmarkFolderModel::~BookmarkFolderModel()
{
    m_bookmarkMgr->m_root.parent = nullptr;
    delete m_root;
}

QModelIndex BookmarkFolderModel::index(int row, int column, const QModelIndex &parent) const
{
    if (parent.isValid() && column != 0)
        return QModelIndex();

    // Attempt to find child
    BookmarkFolder *parentFolder = getItem(parent);
    BookmarkFolder *child = nullptr;
    if (uint64_t(row) < parentFolder->folders.size())
    {
        auto it = parentFolder->folders.begin();
        std::advance(it, row);
        child = *it;
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

    BookmarkFolder *child = getItem(index);
    BookmarkFolder *parent = child->parent;
    if (parent == nullptr)
        return QModelIndex();

    return createIndex(parent->treePosition, 0, parent);
}

int BookmarkFolderModel::rowCount(const QModelIndex &parent) const
{
    return getItem(parent)->folders.size();
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
        BookmarkFolder *folder = getItem(index);
        return folder->name;
    }
    return QVariant();
}

bool BookmarkFolderModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (index.isValid() && role == Qt::EditRole)
    {
        BookmarkFolder *folder = getItem(index);
        folder->name = value.toString();
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
    BookmarkFolder *parentFolder = getItem(parent);
    if (!parentFolder)
        return false;
    //TODO: implement ability to move position of folder to row + i
    beginInsertRows(parent, row, row + count - 1);
    for (int i = 0; i < count; ++i)
        m_bookmarkMgr->addFolder(QString("New Folder %1").arg(i), parentFolder->id);
    endInsertRows();
    return true;
}

bool BookmarkFolderModel::insertColumns(int column, int count, const QModelIndex &parent)
{
    beginInsertColumns(parent, column, column + count - 1);
    // FIXME: Implement me!
    endInsertColumns();
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

bool BookmarkFolderModel::removeColumns(int column, int count, const QModelIndex &parent)
{
    beginRemoveColumns(parent, column, column + count - 1);
    // FIXME: Implement me!
    endRemoveColumns();
    return true;
}

BookmarkFolder *BookmarkFolderModel::getItem(const QModelIndex &index) const
{
    if (index.isValid())
    {
        if (BookmarkFolder *folder = static_cast<BookmarkFolder*>(index.internalPointer()))
            return folder;
    }
    return m_root;
}

