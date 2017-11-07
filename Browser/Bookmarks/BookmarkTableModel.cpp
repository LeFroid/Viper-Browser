#include "BookmarkTableModel.h"
#include "BrowserApplication.h"
#include "FaviconStorage.h"

#include <QByteArray>
#include <QDataStream>
#include <QIODevice>
#include <QMimeData>
#include <QSet>
#include <QDebug>

BookmarkTableModel::BookmarkTableModel(std::shared_ptr<BookmarkManager> bookmarkMgr, QObject *parent) :
    QAbstractTableModel(parent),
    m_bookmarkMgr(bookmarkMgr),
    m_folder(bookmarkMgr->getRoot())
{
}

QVariant BookmarkTableModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation != Qt::Horizontal)
        return QVariant();

    if (role == Qt::DisplayRole)
    {
        switch (section)
        {
            case 0: return "Name";
            case 1: return "Location";
        }
    }

    return QVariant();
}

int BookmarkTableModel::rowCount(const QModelIndex &/*parent*/) const
{
    return (m_folder) ? m_folder->bookmarks.size() : 0;
}

int BookmarkTableModel::columnCount(const QModelIndex &/*parent*/) const
{
    // Fixed number of columns
    return 2;
}

QVariant BookmarkTableModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || !m_folder)
        return QVariant();

    if (uint64_t(index.row()) < m_folder->bookmarks.size())
    {
        auto it = m_folder->bookmarks.begin();
        std::advance(it, index.row());
        Bookmark *b = *it;
        switch (index.column())
        {
            case 0:
            {
                if (role == Qt::EditRole || role == Qt::DisplayRole)
                    return b->name;

                // Try to display favicon next to name
                QIcon favicon = sBrowserApplication->getFaviconStorage()->getFavicon(b->URL);
                if (role == Qt::DecorationRole)
                    return favicon.pixmap(16, 16);
                if (role == Qt::SizeHintRole)
                    return QSize(16, 16);
            }
            case 1: if (role == Qt::DisplayRole || role == Qt::EditRole) return b->URL;
        }
    }
    return QVariant();
}

bool BookmarkTableModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (index.isValid() && role == Qt::EditRole && m_folder->bookmarks.size() > uint64_t(index.row()))
    {
        auto it = m_folder->bookmarks.begin();
        std::advance(it, index.row());
        Bookmark *b = *it;
        if (!b)
            return false;

        // Copy contents of bookmark before modifying, so the old record can be found in the DB
        Bookmark oldValue;
        oldValue.name = b->name;
        oldValue.URL = b->URL;
        switch (index.column())
        {
            case 0: b->name = value.toString(); break;
            case 1: b->URL = value.toString(); break;
        }

        m_bookmarkMgr->updatedBookmark(b, oldValue, m_folder->id);
        emit dataChanged(index, index);
        return true;
    }
    return false;
}

Qt::ItemFlags BookmarkTableModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return Qt::NoItemFlags;

    return Qt::ItemIsEditable | Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled | QAbstractItemModel::flags(index);
}

bool BookmarkTableModel::insertRows(int row, int count, const QModelIndex &parent)
{
    if (!m_folder)
        return false;

    QString name("New Bookmark");
    beginInsertRows(parent, row, row + count - 1);
    for (int i = 0; i < count; ++i)
        m_bookmarkMgr->addBookmark(name, QString("Location %1").arg(i), m_folder, row + i);
    endInsertRows();
    return true;
}

bool BookmarkTableModel::removeRows(int row, int count, const QModelIndex &parent)
{
    if (!m_folder)
        return false;

    std::list<Bookmark*>::iterator it;
    beginRemoveRows(parent, row, row + count - 1);
    for (int i = 0; i < count; ++i)
    {
        it = m_folder->bookmarks.begin();
        std::advance(it, row + i);
        m_bookmarkMgr->removeBookmark(*it, m_folder);
    }
    endRemoveRows();
    return true;
}

QStringList BookmarkTableModel::mimeTypes() const
{
    QStringList types;
    types << "application/x-bookmark-data";
    return types;
}

Qt::DropActions BookmarkTableModel::supportedDropActions() const
{
    return Qt::MoveAction;
}

QMimeData *BookmarkTableModel::mimeData(const QModelIndexList &indexes) const
{
    QMimeData *mimeData = new QMimeData();
    QByteArray encodedData;
    QDataStream stream(&encodedData, QIODevice::WriteOnly);
    for (const QModelIndex &index : indexes)
    {
        if (index.isValid() && m_folder->bookmarks.size() > uint64_t(index.row()))
            stream << index.row();
    }
    mimeData->setData("application/x-bookmark-data", encodedData);
    return mimeData;
}

bool BookmarkTableModel::dropMimeData(const QMimeData *data, Qt::DropAction action, int /*row*/, int /*column*/, const QModelIndex &parent)
{
    if (!data || action != Qt::MoveAction)
        return false;

    if (!data->hasFormat("application/x-bookmark-data"))
        return false;

    // retrieve rows from serialized data       and call m_bookmarkMgr->setBookmarkPosition(bookmark*, bookmark_folder*, new_position)
    QByteArray encodedData = data->data("application/x-bookmark-data");
    QDataStream stream(&encodedData, QIODevice::ReadOnly);

    QSet<int> rowNums;
    while (!stream.atEnd())
    {
        int rowNum;
        stream >> rowNum;
        rowNums << rowNum;
    }

    // Shift row positions
    int currentRow = parent.row();
    beginResetModel();
    for (int r : rowNums)
    {
        if (uint64_t(r) >= m_folder->bookmarks.size())
            continue;

        qDebug() << "Moving bookmark from row " << r << " to row " << parent.row();

        auto it = m_folder->bookmarks.begin();
        std::advance(it, r);
        m_bookmarkMgr->setBookmarkPosition(*it, m_folder, currentRow++);
    }
    endResetModel();

    return true;
}

void BookmarkTableModel::setCurrentFolder(BookmarkFolder *folder)
{
    beginResetModel();
    m_folder = folder;
    endResetModel();
}
