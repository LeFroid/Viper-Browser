#include "BookmarkManager.h"
#include "BookmarkNode.h"

#include <deque>
#include <iterator>
#include <cstdint>
#include <QSqlError>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QDebug>

BookmarkManager::BookmarkManager(const QString &databaseFile) :
    DatabaseWorker(databaseFile, "Bookmarks"),
    m_rootNode(std::make_unique<BookmarkNode>(BookmarkNode::Folder, QString("Bookmarks")))
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
        if (child->getType() == BookmarkNode::Folder && child->getName().compare("Bookmarks Bar") == 0)
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
}

void BookmarkManager::insertBookmark(const QString &name, const QString &url, BookmarkNode *folder, int position)
{
    if (!folder)
        folder = getBookmarksBar();

    if (position < 0 || position > folder->getNumChildren())
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
}

bool BookmarkManager::isBookmarked(const QString &url)
{
    QSqlQuery query(m_database);
    query.prepare("SELECT FolderID FROM Bookmarks WHERE URL = (:url)");
    query.bindValue(":url", url);
    return (query.exec() && query.next());
}

void BookmarkManager::removeBookmark(const QString &url)
{
    // Search DB rather than traverse tree
    QSqlQuery query(m_database);
    query.prepare("SELECT FolderID FROM Bookmarks WHERE URL = (:url)");
    query.bindValue(":url", url);
    if (!query.exec() || !query.first())
        qDebug() << "[Warning]: In BookmarkManager::removeBookmark(..) - could not search database for bookmark with url " << url;

    int folderId = query.value(0).toInt();
    BookmarkNode *folder = findFolder(folderId);
    if (folder == nullptr)
    {
        qDebug() << "[Warning]: In BookmarkManager::removeBookmark(..) - found bookmark, could not find folder it belongs to";
        return;
    }

    for (auto &node : folder->m_children)
    {
        BookmarkNode *b = node.get();
        if (b->m_url.compare(url, Qt::CaseInsensitive) == 0)
        {
            if (!removeBookmarkFromDB(b))
                qDebug() << "Could not remove bookmark from DB";

            folder->removeNode(b);
            break;
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
        parent->removeNode(item);
}

void BookmarkManager::removeFolder(BookmarkNode *folder)
{
    if (!folder || folder == m_rootNode.get())
        return;

    // Delete node and all sub nodes from the database
    QSqlQuery query(m_database);
    query.prepare("DELETE FROM Bookmarks WHERE FolderID = (:id) OR ParentID = (:parentID)");

    int folderId = folder->getFolderId();
    query.bindValue(":id", folderId);
    query.bindValue(":parentID", folderId);
    if (!query.exec())
    {
        qDebug() << "[Warning]: Could not remove bookmarks from database in BookmarkManager::removeFolder. Message: "
                 << query.lastError().text();
    }

    // Recursively delete sub-folders from database
    for (auto &node : folder->m_children)
    {
        BookmarkNode *b = node.get();
        if (b->getType() == BookmarkNode::Folder)
            removeFolder(b);
    }

    // Remove folder from parent
    if (BookmarkNode *parent = folder->getParent())
        parent->removeNode(folder);
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
    return folder;
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

bool BookmarkManager::isValidFolderID(int id)
{
    QSqlQuery query(m_database);
    query.prepare("SELECT ParentID FROM Bookmarks WHERE FolderID = (:id)");
    query.bindValue(":id", id);
    return (query.exec() && query.last());
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

BookmarkNode *BookmarkManager::findFolder(int id)
{
    // Check database to ensure folder id is valid before performing a BFS
    if (!isValidFolderID(id))
        return nullptr;

    std::deque<BookmarkNode*> queue;
    queue.push_back(m_rootNode.get());

    // BFS until the folder with given ID is found, or all folders are searched
    while (!queue.empty())
    {
        BookmarkNode *currNode = queue.front();
        if (!currNode)
        {
            queue.pop_front();
            continue;
        }

        if (currNode->getFolderId() == id)
            return currNode;

        for (auto &node : currNode->m_children)
        {
            BookmarkNode *nodePtr = node.get();
            if (nodePtr->getType() == BookmarkNode::Folder)
                queue.push_back(nodePtr);
        }

        queue.pop_front();
    }

    return nullptr;
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
                  "SELECT Position FROM Bookmarks WHERE URL = (:url)"));
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

    // Insert bookmarks bar folder
    BookmarkNode *bookmarkBar = addFolder("Bookmarks Bar", m_rootNode.get());

    // Insert bookmark for search engine
    query.prepare("INSERT OR IGNORE INTO Bookmarks(FolderID, ParentID, Type, Name, URL, Position) VALUES(:folderID, :parentID, :type, :name, :url, :position)");
    query.bindValue(":folderID", bookmarkBar->getFolderId());
    query.bindValue(":parentID", bookmarkBar->getFolderId());
    query.bindValue(":type", QVariant::fromValue(BookmarkNode::Bookmark));
    query.bindValue(":name", "Search Engine");
    query.bindValue(":url", "https://www.ixquick.com/");
    query.bindValue(":position", 0);
    if (!query.exec())
        qDebug() << "Error inserting bookmark. Message: " << query.lastError().text();
}

void BookmarkManager::load()
{
    loadFolder(m_rootNode.get());
}
