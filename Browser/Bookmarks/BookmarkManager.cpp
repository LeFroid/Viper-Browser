#include "BookmarkManager.h"
#include "BookmarkNode.h"

#include <deque>
#include <iterator>
#include <cstdint>
#include <QSqlError>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QDebug>

BookmarkManager::BookmarkManager(const QString &databaseFile, QObject *parent) :
    QObject(parent),
    DatabaseWorker(databaseFile, "Bookmarks"),
    m_rootNode(std::make_unique<BookmarkNode>(BookmarkNode::Folder, QLatin1String("Bookmarks"))),
    m_nodeList()
{
}

BookmarkManager::~BookmarkManager()
{
}

BookmarkNode *BookmarkManager::getRoot()
{
    return m_rootNode.get();
}

BookmarkNode *BookmarkManager::getBookmarksBar()
{
    int numChildren = m_rootNode->getNumChildren();
    for (int i = 0; i < numChildren; ++i)
    {
        BookmarkNode *child = m_rootNode->getNode(i);
        if (child->getType() == BookmarkNode::Folder
                && child->getName().compare(QLatin1String("Bookmarks Bar")) == 0)
            return child;
    }

    // Settle for root node if could not find bookmarks bar
    return m_rootNode.get();
}

BookmarkNode *BookmarkManager::addFolder(const QString &name, BookmarkNode *parent)
{
    // If parent cannot be found then attach the new folder directly to the root folder
    if (!parent)
    {
        qDebug() << "[Warning]: BookmarkManager::addFolder(..) - Could not find parent folder for new folder \"" << name
                 << "\", setting parent folder to root. ";
        parent = m_rootNode.get();
    }

    // Set position of folder to the bottom of its parent folder
    int position = parent->getNumChildren();

    // Determine which ID the folder will be assigned to
    int folderId = 0;
    QSqlQuery query(m_database);
    query.prepare("SELECT MAX(FolderID) FROM Bookmarks");
    if (!query.exec())
        qDebug() << "[Error]: In BookmarkManager::addFolder(..) - could not fetch max folder id value from database";
    else if (query.first())
        folderId = query.value(0).toInt() + 1;

    query.prepare("INSERT INTO Bookmarks(FolderID, ParentID, Type, Name, Position) VALUES (:folderID, :parentID, :type, :name, :position)");
    query.bindValue(":folderID", folderId);
    query.bindValue(":parentID", parent->getFolderId());
    query.bindValue(":type", static_cast<int>(BookmarkNode::Folder));
    query.bindValue(":name", name);
    query.bindValue(":position", position);
    if (!query.exec())
        qDebug() << "[Error]: In BookmarkManager::addFolder(..) - error inserting new bookmark folder into database. Message: " << query.lastError().text();

    // Append bookmark folder to parent
    BookmarkNode *f = parent->appendNode(std::make_unique<BookmarkNode>(BookmarkNode::Folder, name));
    f->setFolderId(folderId);
    f->setIcon(QIcon::fromTheme("folder"));

    onBookmarksChanged();

    return f;
}

void BookmarkManager::appendBookmark(const QString &name, const QString &url, BookmarkNode *folder)
{
    // If parent folder not specified, set to root folder
    if (!folder)
        folder = getBookmarksBar();

    // Create new bookmark
    BookmarkNode *b = folder->appendNode(std::make_unique<BookmarkNode>(BookmarkNode::Bookmark, name));
    b->setURL(url);

    // Add bookmark to the database
    if (!addBookmarkToDB(b, folder))
        qDebug() << "[Warning]: Could not insert new bookmark into the database";

    onBookmarksChanged();
}

void BookmarkManager::insertBookmark(const QString &name, const QString &url, BookmarkNode *folder, int position)
{
    if (!folder)
        folder = getBookmarksBar();

    if (position < 0 || position >= folder->getNumChildren())
    {
        appendBookmark(name, url, folder);
        return;
    }

    // Create new bookmark        
    BookmarkNode *b = folder->insertNode(std::make_unique<BookmarkNode>(BookmarkNode::Bookmark, name), position);
    b->setURL(url);

    // Update positions of items in same folder
    QSqlQuery query(m_database);
    query.prepare("UPDATE Bookmarks SET Position = Position + 1 WHERE ParentID = (:parentID) AND Position >= (:position)");
    query.bindValue(":parentID", folder->getFolderId());
    query.bindValue(":position", position);
    if (!query.exec())
        qDebug() << "[Warning]: Could not update bookmark positions when calling insertBookmark()";

    // Add bookmark to the database
    if (!addBookmarkToDB(b, folder))
        qDebug() << "[Warning]: Could not insert new bookmark into the database";

    onBookmarksChanged();
}

bool BookmarkManager::isBookmarked(const QString &url)
{
    if (url.isEmpty())
        return false;

    for (BookmarkNode *node : m_nodeList)
    {
        if (node->m_url.compare(url) == 0)
            return true;
    }

    return false;
}

void BookmarkManager::removeBookmark(const QString &url)
{
    if (url.isEmpty())
        return;

    for (BookmarkNode *node : m_nodeList)
    {
        // Bookmark URLs are unique, once match is found, remove bookmark and return
        if (node->m_url.compare(url) == 0)
        {
            if (!removeBookmarkFromDB(node))
                qDebug() << "Could not remove bookmark from DB";
            if (BookmarkNode *parent = node->getParent())
                parent->removeNode(node);
            onBookmarksChanged();
            return;
        }
    }
}

void BookmarkManager::removeBookmark(BookmarkNode *item)
{
    if (!item)
        return;

    // Remove from DB, then from mparent
    if (!removeBookmarkFromDB(item))
        qDebug() << "Could not remove bookmark from DB";

    if (BookmarkNode *parent = item->getParent())
    {
        parent->removeNode(item);
        onBookmarksChanged();
    }
}

void BookmarkManager::removeFolder(BookmarkNode *folder)
{
    if (!folder || folder == m_rootNode.get())
        return;

    // Delete node and all sub nodes from the database
    QSqlQuery query(m_database);
    query.prepare(QLatin1String("DELETE FROM Bookmarks WHERE FolderID = (:id) OR ParentID = (:parentID)"));

    // Iteratively remove the folder and its subfolders from the database
    std::deque<BookmarkNode*> queue;
    queue.push_back(folder);
    while (!queue.empty())
    {
        BookmarkNode *n = queue.front();

        int folderId = n->getFolderId();
        query.bindValue(QLatin1String(":id"), folderId);
        query.bindValue(QLatin1String(":parentID"), folderId);
        if (!query.exec())
        {
            qDebug() << "[Warning]: In BookmarkManager::removeFolder - could not remove bookmarks from database. Error message: "
                     << query.lastError().text();
        }

        for (auto &child : n->m_children)
        {
            BookmarkNode *subNode = child.get();
            if (subNode->getType() == BookmarkNode::Folder)
                queue.push_back(subNode);
        }

        queue.pop_front();
    }

    // Remove folder from its parent
    if (BookmarkNode *parent = folder->getParent())
        parent->removeNode(folder);

    // Update UI elements and bookmark list
    onBookmarksChanged();
}

void BookmarkManager::setNodePosition(BookmarkNode *node, int position)
{
    if (!node)
        return;

    // Get parent node
    BookmarkNode *parent = node->getParent();
    if (!parent)
        return;

    // Make sure the position is valid
    if (position < 0 || position >= parent->getNumChildren())
        return;

    // Ensure bookmark belongs to the folder, and get its current position
    int oldPos = 0;
    auto it = parent->m_children.begin();
    for (; it != parent->m_children.end(); ++it)
    {
        if (it->get() == node)
            break;
        ++oldPos;
    }
    if (it == parent->m_children.end() || oldPos == position)
        return;

    // Update database
    QSqlQuery query(m_database);

    // If position is being shifted closer to the root (ie new index < old index), increment position of items between old and new positions.
    if (position < oldPos)
    {
        query.prepare("UPDATE Bookmarks SET Position = Position + 1 WHERE ParentID = (:parentID) AND Position >= (:posNew) AND Position < (:posOld)");
    }
    // If position is being shifted further down, decrement position of items between old and new positions
    else
    {
        query.prepare("UPDATE Bookmarks SET Position = Position - 1 WHERE ParentID = (:parentID) AND Position <= (:posNew) AND Position > (:posOld)");
    }
    query.bindValue(":parentID", parent->getFolderId());
    query.bindValue(":posNew", position);
    query.bindValue(":posOld", oldPos);
    if (!query.exec())
        qDebug() << "[Warning]: In BookmarkManager::setNodePosition - could not update bookmark positions in database. "
                    "Error message: " << query.lastError().text();

    // Adjust position of the actual node in the DB
    switch (node->getType())
    {
        case BookmarkNode::Folder:
            query.prepare("UPDATE Bookmarks SET Position = (:posNew) WHERE FolderID = (:folderID) AND ParentID = (:parentID)");
            query.bindValue(":posNew", position);
            query.bindValue(":folderID", node->getFolderId());
            query.bindValue(":parentID", parent->getFolderId());
            break;
        case BookmarkNode::Bookmark:
            query.prepare("UPDATE Bookmarks SET Position = (:posNew) WHERE URL = (:url)");
            query.bindValue(":posNew", position);
            query.bindValue(":url", node->getURL());
            break;
    }
    if (!query.exec())
        qDebug() << "[Warning]: In BookmarkManager::setNodePosition - could not update position of bookmark. "
                    "Error message: " << query.lastError().text();

    // Adjust position of node in bookmark tree
    if (position > oldPos)
        ++position;
    static_cast<void>(parent->insertNode(std::make_unique<BookmarkNode>(std::move(*node)), position));
    parent->removeNode(node);

    onBookmarksChanged();
}

BookmarkNode *BookmarkManager::setFolderParent(BookmarkNode *folder, BookmarkNode *newParent)
{
    // Do nothing if pointers are invalid, if they are the same node, or if this is an attempt to move the root node
    if (!folder || !newParent || folder == newParent || folder == m_rootNode.get())
        return folder;

    // Also ensure that the node being moved is in fact a folder
    if (folder->getType() != BookmarkNode::Folder)
        return folder;

    // Check that the new parent is not theold parent
    BookmarkNode *oldParent = folder->getParent();
    if (oldParent == newParent)
        return folder;

    // Determine old position of the folder
    int oldFolderPos = 0;
    for (auto &node : oldParent->m_children)
    {
        BookmarkNode *n = node.get();
        if (n == folder)
            break;
        ++oldFolderPos;
    }

    // Update database
    QSqlQuery query(m_database);

    // Update parent folder in the database
    query.prepare("UPDATE Bookmarks SET ParentID = (:newParentID), Position = (:newPos) WHERE FolderID = (:folderID) AND ParentID = (:oldParentID)");
    query.bindValue(":newParentID", newParent->getFolderId());
    query.bindValue(":newPos", newParent->getNumChildren());
    query.bindValue(":folderID", folder->getFolderId());
    query.bindValue(":oldParentID", oldParent->getFolderId());
    if (!query.exec())
        qDebug() << "[Warning]: In BookmarkManager::setFolderParent - could not change parent of folder that was to be moved. "
                    "Error message: " << query.lastError().text();

    // Update positions of nodes
    query.prepare("UPDATE Bookmarks SET Position = Position - 1 WHERE ParentID = (:parentID) AND Position > (:position)");
    query.bindValue(":parentID", oldParent->getFolderId());
    query.bindValue(":position", oldFolderPos);
    if (!query.exec())
        qDebug() << "[Warning]: In BookmarkManager::setFolderParent - could not update positions of nodes in database. "
                    "Error message: " << query.lastError().text();

    // Remove folder from old parent, add to new parent and reload its sub-nodes from the DB
    QString folderName = folder->getName();
    QIcon folderIcon = folder->getIcon();
    int folderId = folder->getFolderId();
    oldParent->removeNode(folder);

    folder = newParent->appendNode(std::make_unique<BookmarkNode>(BookmarkNode::Folder, folderName));
    folder->setFolderId(folderId);
    folder->setIcon(folderIcon);
    loadFolder(folder);

    onBookmarksChanged();

    return folder;
}

BookmarkNode *BookmarkManager::getBookmark(const QString &url)
{
    if (url.isEmpty())
        return nullptr;

    for (BookmarkNode *node : m_nodeList)
    {
        if (node->getType() == BookmarkNode::Bookmark
                && node->getURL().compare(url) == 0)
            return node;
    }

    return nullptr;
}

void BookmarkManager::updatedBookmark(BookmarkNode *bookmark, BookmarkNode &oldValue, int folderID)
{
    if (!bookmark)
        return;

    // Update database
    QSqlQuery query(m_database);
    if (oldValue.m_url == bookmark->m_url)
    {
        query.prepare("UPDATE Bookmarks SET Name = (:newName) WHERE URL = (:url)");
        query.bindValue(":newName", bookmark->getName());
        query.bindValue(":url", bookmark->getURL());
        if (!query.exec())
            qDebug() << "[Warning]: BookmarkManager::updatedBookmark(..) - Could not modify bookmark name. Error message: "
                     << query.lastError().text();
    }
    else
    {
        // If URL has changed, remove the old record and insert the bookmark as a new one
        int position = 0;
        query.prepare("SELECT Position FROM Bookmarks WHERE URL = (:url)");
        query.bindValue(":url", oldValue.getURL());
        if (query.exec() && query.next())
        {
            position = query.value(0).toInt();
        }
        else
            qDebug() << "[Warning]: BookmarkManager::updatedBookmark(..) - Could not fetch position of bookmark with "
                        "URL " << oldValue.getURL() << ". Error message: " << query.lastError().text();
        query.prepare("DELETE FROM Bookmarks WHERE URL = (:url)");
        query.bindValue(":url", oldValue.getURL());
        static_cast<void>(query.exec());
        query.prepare("INSERT INTO Bookmarks(FolderID, ParentID, Type, Name, URL, Position) VALUES(:folderID, :parentID, :type, :name, :url, :position)");
        query.bindValue(":folderID", folderID);
        query.bindValue(":parentID", folderID);
        query.bindValue(":type", static_cast<int>(bookmark->getType()));
        query.bindValue(":name", bookmark->getName());
        query.bindValue(":url", bookmark->getURL());
        query.bindValue(":position", position);
        if (!query.exec())
            qDebug() << "[Warning]: BookmarkManager::updatedBookmark(..) - Could not insert updated bookmark to database. "
                        "Error message: " << query.lastError().text();
    }
}

void BookmarkManager::updatedFolderName(BookmarkNode *folder)
{
    if (!folder)
        return;
    if (folder->getFolderId() < 0)
        return;

    // Update database
    QSqlQuery query(m_database);
    query.prepare("UPDATE Bookmarks SET Name = (:name) WHERE FolderID = (:folderID) AND ParentID = (:parentID)");
    query.bindValue(":name", folder->getName());
    query.bindValue(":folderID", folder->getFolderId());
    int parentId = -1;
    BookmarkNode *parent = folder->getParent();
    if (parent != nullptr)
        parentId = parent->getFolderId();
    query.bindValue(":parentID", parentId);
    if (!query.exec())
        qDebug() << "Error updating name of bookmark folder in database. Message: " << query.lastError().text();
}

void BookmarkManager::loadFolder(BookmarkNode *folder)
{
    if (!folder)
    {
        qDebug() << "Error: null pointer passed to BookmarkManager::loadFolder";
        return;
    }

    QSqlQuery query(m_database);
    query.prepare(QStringLiteral("SELECT FolderID, Type, Name, URL FROM Bookmarks WHERE ParentID = (:id) ORDER BY Position ASC"));

    // Iteratively load folder and all of its subfolders
    std::deque<BookmarkNode*> subFolders;
    subFolders.push_back(folder);
    while (!subFolders.empty())
    {
        BookmarkNode *n = subFolders.front();
        query.bindValue(QStringLiteral(":id"), n->getFolderId());
        if (!query.exec())
        {
            qDebug() << "Error loading bookmarks for folder " << n->getName() << ". Message: " << query.lastError().text();
            continue;
        }

        while (query.next())
        {
            BookmarkNode::NodeType nodeType = static_cast<BookmarkNode::NodeType>(query.value(1).toInt());
            BookmarkNode *subNode = n->appendNode(std::make_unique<BookmarkNode>(nodeType, query.value(2).toString()));
            switch (nodeType)
            {
                // Load folder data
                case BookmarkNode::Folder:
                    subNode->setFolderId(query.value(0).toInt());
                    subNode->setIcon(QIcon::fromTheme(QStringLiteral("folder")));
                    subFolders.push_back(subNode);
                    break;
                // Load bookmark data
                case BookmarkNode::Bookmark:
                    subNode->setURL(query.value(3).toString());
                    break;
            }
        }

        subFolders.pop_front();
    }
}

bool BookmarkManager::addBookmarkToDB(BookmarkNode *bookmark, BookmarkNode *folder)
{
    QSqlQuery query(m_database);    
    query.prepare(QStringLiteral("INSERT OR REPLACE INTO Bookmarks(FolderID, ParentID, Type, Name, URL, Position) "
                  "VALUES(:folderID, :parentID, :type, :name, :url, :position)"));
    int folderId = folder->getFolderId();
    query.bindValue(QStringLiteral(":folderID"), folderId);
    query.bindValue(QStringLiteral(":parentID"), folderId);
    query.bindValue(QStringLiteral(":type"), static_cast<int>(bookmark->getType()));
    query.bindValue(QStringLiteral(":name"), bookmark->getName());
    query.bindValue(QStringLiteral(":url"), bookmark->getURL());
    query.bindValue(QStringLiteral(":position"), folder->getNumChildren());
    return query.exec();
}

bool BookmarkManager::removeBookmarkFromDB(BookmarkNode *bookmark)
{
    bool ok = true;

    QSqlQuery query(m_database);
    // Remove bookmark and update positions of other nodes in same folder
    query.prepare(QStringLiteral("UPDATE Bookmarks SET Position = Position - 1 WHERE FolderID = (:folderId) AND Position > "
                  "(SELECT Position FROM Bookmarks WHERE URL = (:url))"));
    query.bindValue(QStringLiteral(":folderId"), bookmark->getFolderId());

    // Error checking not needed for this query
    static_cast<void>(query.exec());

    query.prepare(QStringLiteral("DELETE FROM Bookmarks WHERE URL = (:url)"));
    query.bindValue(QStringLiteral(":url"), bookmark->getURL());
    if (!query.exec())
    {
        qDebug() << "[Warning]: In BookmarkManager::removeBookmarkFromDB(..) - DB Error: " << query.lastError().text();
        ok = false;
    }

    return ok;
}

void BookmarkManager::onBookmarksChanged()
{
    resetBookmarkList();
    emit bookmarksChanged();
}

void BookmarkManager::resetBookmarkList()
{
    m_nodeList.clear();

    std::deque<BookmarkNode*> queue;
    queue.push_back(m_rootNode.get());
    while (!queue.empty())
    {
        BookmarkNode *n = queue.front();

        for (auto &node : n->m_children)
        {   
            BookmarkNode *childNode = node.get();
            m_nodeList.push_back(childNode);

            if (childNode->getType() == BookmarkNode::Folder)
                queue.push_back(childNode);
        }

        queue.pop_front();
    }
}

bool BookmarkManager::hasProperStructure()
{
    // Verify existence of Bookmarks table
    return hasTable(QStringLiteral("Bookmarks"));
}

void BookmarkManager::setup()
{
    // Setup table structures
    QSqlQuery query(m_database);
    if (!query.exec("CREATE TABLE Bookmarks(ID INTEGER PRIMARY KEY, FolderID INTEGER, ParentID INTEGER DEFAULT 0, "
               "Type INTEGER DEFAULT 0, Name TEXT, URL TEXT UNIQUE, Position INTEGER DEFAULT 0)"))
            qDebug() << "Error creating table Bookmarks. Message: " << query.lastError().text();

    // Insert root bookmark folder
    int rootFolderId = 0;
    query.prepare("INSERT INTO Bookmarks(FolderID, ParentID, Type, Name) VALUES (:folderID, :parentID, :type, :name)");
    query.bindValue(":folderID", rootFolderId);
    query.bindValue(":parentID", -1);
    query.bindValue(":type", QVariant::fromValue(BookmarkNode::Folder));
    query.bindValue(":name", "Bookmarks");
    if (!query.exec())
            qDebug() << "Error inserting root bookmark folder. Message: " << query.lastError().text();

    m_rootNode->setFolderId(rootFolderId);

    // Insert bookmarks bar folder
    BookmarkNode *bookmarkBar = addFolder("Bookmarks Bar", m_rootNode.get());

    // Insert bookmark for search engine
    appendBookmark(QLatin1String("Search Engine"), QLatin1String("https://www.ixquick.com"), bookmarkBar);
}

void BookmarkManager::load()
{
    loadFolder(m_rootNode.get());
    resetBookmarkList();
}
