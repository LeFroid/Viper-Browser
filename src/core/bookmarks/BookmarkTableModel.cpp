#include "BookmarkTableModel.h"
#include "BookmarkNode.h"
#include "BookmarkManager.h"

#include <deque>
#include <set>

#include <QByteArray>
#include <QDataStream>
#include <QIODevice>
#include <QMimeData>
#include <QSet>
#include <QUrl>
#include <QDebug>

BookmarkTableModel::BookmarkTableModel(BookmarkManager *bookmarkMgr, QObject *parent) :
    QAbstractTableModel(parent),
    m_bookmarkMgr(bookmarkMgr),
    m_folder(bookmarkMgr->getBookmarksBar()),
    m_searchModeOn(false),
    m_searchResults()
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
            case 0: return QString("Name");
            case 1: return QString("Location");
        }
    }

    return QVariant();
}

int BookmarkTableModel::rowCount(const QModelIndex &/*parent*/) const
{
    if (m_searchModeOn)
        return static_cast<int>(m_searchResults.size());

    return m_folder? m_folder->getNumChildren() : 0;
}

int BookmarkTableModel::columnCount(const QModelIndex &/*parent*/) const
{
    // Fixed number of columns
    return 2;
}

QVariant BookmarkTableModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    BookmarkNode *b = getBookmark(index.row());
    if (b == nullptr)
        return QVariant();

    switch (index.column())
    {
        case 0:
        {
            if (role == Qt::EditRole || role == Qt::DisplayRole)
                return b->getName();

            // Try to display icon next to name
            QIcon nodeIcon;
            switch (b->getType())
            {
                case BookmarkNode::Bookmark:
                case BookmarkNode::Folder:
                    nodeIcon = b->getIcon();
                    break;
            }
            if (role == Qt::DecorationRole)
                return nodeIcon.pixmap(16, 16);
            if (role == Qt::SizeHintRole)
                return QSize(16, 16);
            break;
        }
        case 1:
            if (role == Qt::DisplayRole || role == Qt::EditRole)
                return b->getURL().toString(QUrl::FullyEncoded);
            break;
    }

    return QVariant();
}

bool BookmarkTableModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (index.isValid() && role == Qt::EditRole && rowCount() > index.row())
    {
        BookmarkNode *b = getBookmark(index.row());
        QString newValue = value.toString();

        if (b == nullptr || b->getType() != BookmarkNode::Bookmark || newValue.isEmpty())
            return false;

        switch (index.column())
        {
            // Name
            case 0:
            {
                m_bookmarkMgr->setBookmarkName(b, newValue);
                break;
            }
            // URL
            case 1:
            {
                m_bookmarkMgr->setBookmarkURL(b, QUrl::fromUserInput(newValue));
                break;
            }
            default:
                break;
        }

        emit dataChanged(index, index);
        return true;
    }
    return false;
}

Qt::ItemFlags BookmarkTableModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return Qt::NoItemFlags;

    Qt::ItemFlags flags = Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled | QAbstractItemModel::flags(index);

    BookmarkNode *bookmark = getBookmark(index.row());
    if (bookmark != nullptr && bookmark->getType() == BookmarkNode::Bookmark)
        flags |= Qt::ItemIsEditable;

    return flags;
}

bool BookmarkTableModel::insertRows(int row, int count, const QModelIndex &parent)
{
    if (!m_folder || m_searchModeOn)
        return false;

    QString name("New Bookmark");

    beginInsertRows(parent, row, row + count - 1);
    for (int i = 0; i < count; ++i)
        m_bookmarkMgr->insertBookmark(name, QUrl::fromUserInput(QString("file://location %1").arg(i)), m_folder, row + i);
    endInsertRows();

    return true;
}

bool BookmarkTableModel::removeRows(int row, int count, const QModelIndex &parent)
{
    if (!m_folder)
        return false;

    beginRemoveRows(parent, row, row + count - 1);

    for (int i = 0; i < count; ++i)
    {
        BookmarkNode *n = getBookmark(row + i);
        if (n != nullptr)
            m_bookmarkMgr->removeBookmark(n);
    }

    if (m_searchModeOn)
        m_searchResults.erase(m_searchResults.begin() + row, m_searchResults.begin() + row + count);

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
    std::set<int> rows;
    for (const QModelIndex &index : indexes)
    {
        if (rows.find(index.row()) != rows.end())
            continue;

        if (index.isValid())
        {
            if (BookmarkNode *n = getBookmark(index.row()))
            {
                stream << n;
                rows.insert(index.row());
            }
        }
    }
    mimeData->setData(QStringLiteral("application/x-bookmark-data"), encodedData);
    return mimeData;
}

bool BookmarkTableModel::dropMimeData(const QMimeData *data, Qt::DropAction action, int /*row*/, int /*column*/, const QModelIndex &parent)
{
    if (m_searchModeOn)
        return false;

    if (!data || action != Qt::MoveAction)
        return false;

    if (!data->hasFormat(QStringLiteral("application/x-bookmark-data")))
        return false;

    // retrieve rows from serialized data
    QByteArray encodedData = data->data(QStringLiteral("application/x-bookmark-data"));
    QDataStream stream(&encodedData, QIODevice::ReadOnly);

    std::vector<BookmarkNode*> nodes;
    while (!stream.atEnd())
    {
        BookmarkNode *node;
        stream >> node;
        nodes.push_back(node);
    }

    // Shift row positions
    int newRow = parent.row();
    bool needUpdateModel = false;

    beginResetModel();
    for (BookmarkNode *n : nodes)
    {
        if (n->getType() == BookmarkNode::Folder)
            needUpdateModel = true;
        m_bookmarkMgr->setBookmarkPosition(n, newRow);
        ++newRow;
    }
    endResetModel();

    if (needUpdateModel)
        emit movedFolder();

    return true;
}

void BookmarkTableModel::setCurrentFolder(BookmarkNode *folder)
{
    beginResetModel();
    m_folder = folder;
    m_searchModeOn = false;
    m_searchResults.clear();
    endResetModel();
}

BookmarkNode *BookmarkTableModel::getCurrentFolder() const
{
    return m_folder;
}

bool BookmarkTableModel::isInSearchMode() const
{
    return m_searchModeOn;
}

void BookmarkTableModel::searchFor(const QString &text)
{
    beginResetModel();

    m_searchResults.clear();

    // If empty string, return to normal view
    if (text.isEmpty())
    {
        m_searchModeOn = false;
        endResetModel();
        return;
    }

    m_searchModeOn = true;

    // Make term lowercase (case-insensitive search)
    QString query = text.toLower();

    // Iterate through bookmark collection for any bookmark titles or urls containing the search term
    for (auto it = m_bookmarkMgr->begin(); it != m_bookmarkMgr->end(); ++it)
    {
        BookmarkNode *node = *it;
        if (node->getName().toLower().contains(query)
                || node->getURL().toString().toLower().contains(query))
        {
            m_searchResults.push_back(node);
        }
    }

    endResetModel();
}

BookmarkNode *BookmarkTableModel::getBookmark(int row) const
{
    if (row >= rowCount())
        return nullptr;

    if (!m_searchModeOn && m_folder == nullptr)
        return nullptr;

    return m_searchModeOn ? m_searchResults.at(row) : m_folder->getNode(row);
}
