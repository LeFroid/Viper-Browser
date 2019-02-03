#include "BrowserApplication.h"
#include "BookmarkNode.h"
#include "BookmarkManager.h"
#include "BookmarkStore.h"
#include "FaviconStore.h"

#include <deque>
#include <iterator>
#include <cstdint>
#include <QtConcurrent>
#include <QFuture>
#include <QSqlError>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QDebug>

BookmarkStore::BookmarkStore(const QString &databaseFile, QObject *parent) :
    QObject(parent),
    DatabaseWorker(databaseFile, QLatin1String("Bookmarks")),
    m_rootNode(std::make_unique<BookmarkNode>(BookmarkNode::Folder, QLatin1String("Bookmarks"))),
    m_nodeManager(new BookmarkManager(this))
{
    connect(m_nodeManager, &BookmarkManager::bookmarkChanged, this, &BookmarkStore::onBookmarkChanged);
    connect(m_nodeManager, &BookmarkManager::bookmarkCreated, this, &BookmarkStore::onBookmarkCreated);
    connect(m_nodeManager, &BookmarkManager::bookmarkDeleted, this, &BookmarkStore::onBookmarkDeleted);
}

BookmarkStore::~BookmarkStore()
{
    //save();
}

BookmarkManager *BookmarkStore::getNodeManager() const
{
    return m_nodeManager;
}

void BookmarkStore::onBookmarkChanged(const BookmarkNode *node)
{
    BookmarkNode *parent = node->getParent();
    if (!parent)
        parent = m_rootNode.get();

    QSqlQuery query(m_database);
    query.prepare(QLatin1String("UPDATE Bookmarks SET ParentID = (:parentID), Name = (:name), URL = (:url), "
                                "Shortcut = (:shortcut), Position = (:position) WHERE ID = (:id)"));

    query.bindValue(QLatin1String(":parentID"), parent->getUniqueId());
    query.bindValue(QLatin1String(":name"), node->getName());
    query.bindValue(QLatin1String(":url"), node->getURL());
    query.bindValue(QLatin1String(":shortcut"), node->getShortcut());
    query.bindValue(QLatin1String(":position"), node->getPosition());
    query.bindValue(QLatin1String(":id"), node->getUniqueId());

    if (!query.exec())
    {
        qWarning() << "BookmarkStore::onBookmarkChanged - could not update bookmark node. Error: "
                   << query.lastError().text();
    }

    query.prepare(QLatin1String("UPDATE Bookmarks SET Position = Position + 1 WHERE ParentID = (:parentID) "
                                "AND Position >= (:position) AND ID != (:id)"));
    query.bindValue(QLatin1String(":parentID"), parent->getUniqueId());
    query.bindValue(QLatin1String(":position"), node->getPosition());
    query.bindValue(QLatin1String(":id"), node->getUniqueId());
    if (!query.exec())
    {
        qWarning() << "BookmarkStore::onBookmarkCreated - could not update bookmark positions. Error: "
                   << query.lastError().text();
    }
}

void BookmarkStore::onBookmarkCreated(const BookmarkNode *node)
{
    BookmarkNode *parent = node->getParent();
    if (!parent)
        parent = m_rootNode.get();

    QSqlQuery query(m_database);
    query.prepare(QLatin1String("INSERT OR REPLACE INTO Bookmarks(ID, ParentID, Type, Name, URL, Position) "
                  "VALUES(:id, :parentID, :type, :name, :url, :position)"));

    query.bindValue(QLatin1String(":id"), node->getUniqueId());
    query.bindValue(QLatin1String(":parentID"), parent->getUniqueId());
    query.bindValue(QLatin1String(":type"), static_cast<int>(node->getType()));
    query.bindValue(QLatin1String(":name"), node->getName());
    query.bindValue(QLatin1String(":url"), node->getURL());
    query.bindValue(QLatin1String(":position"), node->getPosition());

    if (!query.exec())
    {
        qWarning() << "BookmarkStore::onBookmarkCreated - could not create bookmark node. Error: "
                   << query.lastError().text();
    }

    query.prepare(QLatin1String("UPDATE Bookmarks SET Position = Position + 1 WHERE ParentID = (:parentID) AND Position >= (:position)"));
    query.bindValue(QLatin1String(":parentID"), parent->getUniqueId());
    query.bindValue(QLatin1String(":position"), node->getPosition());
    if (!query.exec())
    {
        qWarning() << "BookmarkStore::onBookmarkCreated - could not update bookmark positions. Error: "
                   << query.lastError().text();
    }
}

void BookmarkStore::onBookmarkDeleted(int uniqueId, int parentId, int position)
{
    QSqlQuery query(m_database);
    query.prepare(QLatin1String("DELETE FROM Bookmarks WHERE ID = (:id) OR ParentID = (:id)"));
    query.bindValue(QLatin1String(":id"), uniqueId);
    if (!query.exec())
    {
        qWarning() << "BookmarkStore::onBookmarkDeleted - could not delete bookmark node. Error: "
                   << query.lastError().text();
    }

    // Update positions
    query.prepare(QLatin1String("UPDATE Bookmarks SET Position = Position - 1 WHERE ParentID = (:parentID) AND Position >= (:position)"));
    query.bindValue(QLatin1String(":parentID"), parentId);
    query.bindValue(QLatin1String(":position"), position);
    if (!query.exec())
    {
        qWarning() << "BookmarkStore::onBookmarkDeleted - could not update bookmark positions. Error: "
                   << query.lastError().text();
    }
}

void BookmarkStore::loadFolder(BookmarkNode *folder)
{
    if (!folder)
    {
        qDebug() << "Error: null pointer passed to BookmarkStore::loadFolder";
        return;
    }

    FaviconStore *faviconStore = sBrowserApplication->getFaviconStore();

    QSqlQuery query(m_database);
    query.prepare(QLatin1String("SELECT ID, Type, Name, URL, Shortcut FROM Bookmarks WHERE ParentID = (:id) ORDER BY Position ASC"));

    // Iteratively load folder and all of its subfolders
    std::deque<BookmarkNode*> subFolders;
    subFolders.push_back(folder);
    while (!subFolders.empty())
    {
        BookmarkNode *n = subFolders.front();
        query.bindValue(QLatin1String(":id"), n->getUniqueId());
        if (!query.exec())
        {
            qDebug() << "Error loading bookmarks for folder " << n->getName() << ". Message: " << query.lastError().text();
            continue;
        }

        while (query.next())
        {
            int uniqueId = query.value(0).toInt();

            BookmarkNode::NodeType nodeType = static_cast<BookmarkNode::NodeType>(query.value(1).toInt());
            BookmarkNode *subNode = n->appendNode(std::make_unique<BookmarkNode>(nodeType, query.value(2).toString()));
            subNode->setUniqueId(uniqueId);

            switch (nodeType)
            {
                // Load folder data
                case BookmarkNode::Folder:
                    subNode->setIcon(QIcon::fromTheme(QLatin1String("folder")));
                    subFolders.push_back(subNode);
                    break;
                // Load bookmark data
                case BookmarkNode::Bookmark:
                    subNode->setURL(query.value(3).toString());
                    subNode->setShortcut(query.value(4).toString());
                    subNode->setIcon(faviconStore->getFavicon(subNode->m_url));
                    break;
            }
        }

        subFolders.pop_front();
    }
}

bool BookmarkStore::hasProperStructure()
{
    // Verify existence of Bookmarks table
    return hasTable(QLatin1String("Bookmarks"));
}

void BookmarkStore::setup()
{
    // Setup table structures
    QSqlQuery query(m_database);
    if (!query.exec(QLatin1String("CREATE TABLE Bookmarks(ID INTEGER PRIMARY KEY, ParentID INTEGER DEFAULT 0, "
               "Type INTEGER DEFAULT 0, Name TEXT, URL TEXT, Shortcut TEXT, Position INTEGER DEFAULT 0)")))
            qDebug() << "Error creating table Bookmarks. Message: " << query.lastError().text();

    // Insert root bookmark folder
    query.prepare(QLatin1String("INSERT INTO Bookmarks(ID, ParentID, Type, Name) VALUES (:id, :parentID, :type, :name)"));
    query.bindValue(QLatin1String(":id"), 0);
    query.bindValue(QLatin1String(":parentID"), -1);
    query.bindValue(QLatin1String(":type"), static_cast<int>(BookmarkNode::Folder));
    query.bindValue(QLatin1String(":name"), QLatin1String("Bookmarks"));
    if (!query.exec())
        qDebug() << "Error inserting root bookmark folder. Message: " << query.lastError().text();

    m_rootNode->setUniqueId(0);
    m_nodeManager->setRootNode(m_rootNode.get());

    // Insert bookmarks bar folder
    BookmarkNode *bookmarkBar = m_nodeManager->addFolder(QLatin1String("Bookmarks Bar"), m_rootNode.get());

    // Set root node again so the node manager will fetch the bookmark bar
    m_nodeManager->setRootNode(m_rootNode.get());

    // Insert bookmark for search engine
    m_nodeManager->appendBookmark(QLatin1String("Search Engine"), QUrl(QLatin1String("https://www.startpage.com")), bookmarkBar);
}

void BookmarkStore::save()
{
    if (!m_database.transaction())
    {
        qWarning() << "BookmarkStore::save - could not start transaction";
        return;
    }

    if (!exec(QLatin1String("DELETE FROM Bookmarks")))
        qWarning() << "BookmarkStore::save - Could not clear bookmarks table";

    QSqlQuery query(m_database);
    query.prepare(QLatin1String("INSERT INTO Bookmarks(ID, ParentID, Type, Name, URL, Shortcut, Position) "
                                "VALUES(:id, :parentID, :type, :name, :url, :shortcut, :position)"));

    auto saveNode = [&query](BookmarkNode *node) {
        int parentId = -1;
        if (BookmarkNode *parent = node->getParent())
            parentId = parent->getUniqueId();

        query.bindValue(QLatin1String(":id"), node->getUniqueId());
        query.bindValue(QLatin1String(":parentID"), parentId);
        query.bindValue(QLatin1String(":type"), static_cast<int>(node->getType()));
        query.bindValue(QLatin1String(":name"), node->getName());
        query.bindValue(QLatin1String(":url"), node->getURL());
        query.bindValue(QLatin1String(":shortcut"), node->getShortcut());
        query.bindValue(QLatin1String(":position"), node->getPosition());
        if (!query.exec())
            qWarning() << "Could not save bookmark " << node->getName() << ", id " << node->getUniqueId();
    };

    std::deque<BookmarkNode*> queue;
    queue.push_back(m_rootNode.get());
    while (!queue.empty())
    {
        BookmarkNode *node = queue.front();
        saveNode(node);

        for (auto &child : node->m_children)
        {
            if (child->getType() == BookmarkNode::Folder)
                queue.push_back(child.get());
            else
                saveNode(child.get());
        }

        queue.pop_front();
    }

    if (!m_database.commit())
        qWarning() << "BookmarkStore::save - could not commit transaction";
}

void BookmarkStore::load()
{
    // Check if table structure needs update before loading
    QSqlQuery query(m_database);
    if (query.exec(QLatin1String("PRAGMA table_info(Bookmarks)")))
    {
        bool hasSortcutColumn = false;
        const QString shortcutColumn("Shortcut");

        bool hasFolderIdColumn = false;
        const QString folderIdColumn("FolderID");

        while (query.next())
        {
            QString columnName = query.value(1).toString();
            if (columnName.compare(shortcutColumn) == 0)
                hasSortcutColumn = true;
            else if (columnName.compare(folderIdColumn) == 0)
                hasFolderIdColumn = true;
        }

        if (!hasSortcutColumn)
        {
            if (!query.exec(QLatin1String("ALTER TABLE Bookmarks ADD Shortcut TEXT")))
                qDebug() << "Error updating bookmark table with shortcut column";
        }

        if (hasFolderIdColumn)
        {
            if (!m_database.transaction())
                qWarning() << "BookmarkStore::load - could not start transaction";

            if (!query.exec(QLatin1String("CREATE TABLE Bookmarks_New(ID INTEGER PRIMARY KEY, ParentID INTEGER DEFAULT 0, "
                                          "Type INTEGER DEFAULT 0, Name TEXT, URL TEXT, Shortcut TEXT, Position INTEGER DEFAULT 0)")))
                qWarning() << "BookmarkStore::load - could not update table structure";

            if (!query.exec(QLatin1String("INSERT INTO Bookmarks_New SELECT ID, ParentID, Type, Name, URL, Shortcut, Position FROM Bookmarks")))
                qWarning() << "BookmarkStore::load - could not migrate bookmark data";

            if (!query.exec(QLatin1String("DROP TABLE Bookmarks")))
                qWarning() << "BookmarkStore::load - could not migrate bookmark data";

            if (!query.exec(QLatin1String("ALTER TABLE Bookmarks_New RENAME TO Bookmarks")))
                qWarning() << "BookmarkStore::load - could not migrate bookmark data";

            if (!m_database.commit())
                qWarning() << "BookmarkStore::load - could not commit transaction";
        }
    }

    // Don't load twice
    if (m_rootNode->getNumChildren() == 0)
    {
        m_rootNode->setUniqueId(0);

        loadFolder(m_rootNode.get());

        m_nodeManager->setRootNode(m_rootNode.get());
    }
}
