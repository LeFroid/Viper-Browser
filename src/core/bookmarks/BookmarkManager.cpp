#include "BrowserApplication.h"
#include "BookmarkManager.h"
#include "BookmarkNode.h"
#include "BookmarkNodeManager.h"
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

BookmarkManager::BookmarkManager(const QString &databaseFile, QObject *parent) :
    QObject(parent),
    DatabaseWorker(databaseFile, QLatin1String("Bookmarks")),
    m_rootNode(std::make_unique<BookmarkNode>(BookmarkNode::Folder, QLatin1String("Bookmarks"))),
    m_nodeManager(new BookmarkNodeManager(this))
{
    connect(m_nodeManager, &BookmarkNodeManager::bookmarkChanged, this, &BookmarkManager::onBookmarkChanged);
    connect(m_nodeManager, &BookmarkNodeManager::bookmarkCreated, this, &BookmarkManager::onBookmarkCreated);
    connect(m_nodeManager, &BookmarkNodeManager::bookmarkDeleted, this, &BookmarkManager::onBookmarkDeleted);
}

BookmarkManager::~BookmarkManager()
{
    save();
}

BookmarkNodeManager *BookmarkManager::getNodeManager() const
{
    return m_nodeManager;
}

void BookmarkManager::onBookmarkChanged(const BookmarkNode *node)
{
    BookmarkNode *parent = node->getParent();
    if (!parent)
        parent = m_rootNode.get();

    QSqlQuery query(m_database);
    query.prepare(QLatin1String("UPDATE Bookmarks SET FolderID = (:folderID), ParentID = (:parentID), Name = (:name), URL = (:url), "
                                "Shortcut = (:shortcut), Position = (:position) WHERE ID = (:id)"));

    query.bindValue(QLatin1String(":folderID"), node->getFolderId());
    query.bindValue(QLatin1String(":parentID"), parent->getUniqueId());
    query.bindValue(QLatin1String(":name"), node->getName());
    query.bindValue(QLatin1String(":url"), node->getURL());
    query.bindValue(QLatin1String(":shortcut"), node->getShortcut());
    query.bindValue(QLatin1String(":position"), node->getPosition());
    query.bindValue(QLatin1String(":id"), node->getUniqueId());

    if (!query.exec())
    {
        qWarning() << "BookmarkManager::onBookmarkChanged - could not update bookmark node. Error: "
                   << query.lastError().text();
    }

    query.prepare(QLatin1String("UPDATE Bookmarks SET Position = Position + 1 WHERE ParentID = (:parentID) "
                                "AND Position >= (:position) AND ID != (:id)"));
    query.bindValue(QLatin1String(":parentID"), parent->getUniqueId());
    query.bindValue(QLatin1String(":position"), node->getPosition());
    query.bindValue(QLatin1String(":id"), node->getUniqueId());
    if (!query.exec())
    {
        qWarning() << "BookmarkManager::onBookmarkCreated - could not update bookmark positions. Error: "
                   << query.lastError().text();
    }
}

void BookmarkManager::onBookmarkCreated(const BookmarkNode *node)
{
    BookmarkNode *parent = node->getParent();
    if (!parent)
        parent = m_rootNode.get();

    QSqlQuery query(m_database);
    query.prepare(QLatin1String("INSERT OR REPLACE INTO Bookmarks(ID, FolderID, ParentID, Type, Name, URL, Position) "
                  "VALUES(:id, :folderID, :parentID, :type, :name, :url, :position)"));

    query.bindValue(QLatin1String(":id"), node->getUniqueId());
    query.bindValue(QLatin1String(":folderID"), node->getFolderId());
    query.bindValue(QLatin1String(":parentID"), parent->getUniqueId());
    query.bindValue(QLatin1String(":type"), static_cast<int>(node->getType()));
    query.bindValue(QLatin1String(":name"), node->getName());
    query.bindValue(QLatin1String(":url"), node->getURL());
    query.bindValue(QLatin1String(":position"), node->getPosition());

    if (!query.exec())
    {
        qWarning() << "BookmarkManager::onBookmarkCreated - could not create bookmark node. Error: "
                   << query.lastError().text();
    }

    query.prepare(QLatin1String("UPDATE Bookmarks SET Position = Position + 1 WHERE ParentID = (:parentID) AND Position >= (:position)"));
    query.bindValue(QLatin1String(":parentID"), parent->getUniqueId());
    query.bindValue(QLatin1String(":position"), node->getPosition());
    if (!query.exec())
    {
        qWarning() << "BookmarkManager::onBookmarkCreated - could not update bookmark positions. Error: "
                   << query.lastError().text();
    }
}

void BookmarkManager::onBookmarkDeleted(int uniqueId, int parentId, int position)
{
    QSqlQuery query(m_database);
    query.prepare(QLatin1String("DELETE FROM Bookmarks WHERE ID = (:id) OR ParentID = (:id)"));
    query.bindValue(QLatin1String(":id"), uniqueId);
    if (!query.exec())
    {
        qWarning() << "BookmarkManager::onBookmarkDeleted - could not delete bookmark node. Error: "
                   << query.lastError().text();
    }

    // Update positions
    query.prepare(QLatin1String("UPDATE Bookmarks SET Position = Position - 1 WHERE ParentID = (:parentID) AND Position >= (:position)"));
    query.bindValue(QLatin1String(":parentID"), parentId);
    query.bindValue(QLatin1String(":position"), position);
    if (!query.exec())
    {
        qWarning() << "BookmarkManager::onBookmarkDeleted - could not update bookmark positions. Error: "
                   << query.lastError().text();
    }
}

void BookmarkManager::loadFolder(BookmarkNode *folder)
{
    if (!folder)
    {
        qDebug() << "Error: null pointer passed to BookmarkManager::loadFolder";
        return;
    }

    FaviconStore *faviconStore = sBrowserApplication->getFaviconStore();

    QSqlQuery query(m_database);
    query.prepare(QLatin1String("SELECT ID, FolderID, Type, Name, URL, Shortcut FROM Bookmarks WHERE ParentID = (:id) ORDER BY Position ASC"));

    // Iteratively load folder and all of its subfolders
    std::deque<BookmarkNode*> subFolders;
    subFolders.push_back(folder);
    while (!subFolders.empty())
    {
        BookmarkNode *n = subFolders.front();
        query.bindValue(QLatin1String(":id"), n->getFolderId());
        if (!query.exec())
        {
            qDebug() << "Error loading bookmarks for folder " << n->getName() << ". Message: " << query.lastError().text();
            continue;
        }

        while (query.next())
        {
            int uniqueId = query.value(0).toInt();

            BookmarkNode::NodeType nodeType = static_cast<BookmarkNode::NodeType>(query.value(2).toInt());
            BookmarkNode *subNode = n->appendNode(std::make_unique<BookmarkNode>(nodeType, query.value(3).toString()));
            subNode->setUniqueId(uniqueId);

            switch (nodeType)
            {
                // Load folder data
                case BookmarkNode::Folder:
                    subNode->setFolderId(query.value(1).toInt());
                    subNode->setIcon(QIcon::fromTheme(QLatin1String("folder")));
                    subFolders.push_back(subNode);
                    break;
                // Load bookmark data
                case BookmarkNode::Bookmark:
                    subNode->setFolderId(n->getUniqueId());
                    subNode->setURL(query.value(4).toString());
                    subNode->setShortcut(query.value(5).toString());
                    subNode->setIcon(faviconStore->getFavicon(subNode->m_url));
                    break;
            }
        }

        subFolders.pop_front();
    }
}

bool BookmarkManager::hasProperStructure()
{
    // Verify existence of Bookmarks table
    return hasTable(QLatin1String("Bookmarks"));
}

void BookmarkManager::setup()
{
    // Setup table structures
    QSqlQuery query(m_database);
    if (!query.exec(QLatin1String("CREATE TABLE Bookmarks(ID INTEGER PRIMARY KEY, FolderID INTEGER, ParentID INTEGER DEFAULT 0, "
               "Type INTEGER DEFAULT 0, Name TEXT, URL TEXT, Shortcut TEXT, Position INTEGER DEFAULT 0)")))
            qDebug() << "Error creating table Bookmarks. Message: " << query.lastError().text();

    // Insert root bookmark folder
    query.prepare(QLatin1String("INSERT INTO Bookmarks(ID, FolderID, ParentID, Type, Name) VALUES (:id, :folderID, :parentID, :type, :name)"));
    query.bindValue(QLatin1String(":id"), 0);
    query.bindValue(QLatin1String(":folderID"), 0);
    query.bindValue(QLatin1String(":parentID"), -1);
    query.bindValue(QLatin1String(":type"), static_cast<int>(BookmarkNode::Folder));
    query.bindValue(QLatin1String(":name"), QLatin1String("Bookmarks"));
    if (!query.exec())
        qDebug() << "Error inserting root bookmark folder. Message: " << query.lastError().text();

    m_rootNode->setFolderId(0);
    m_rootNode->setUniqueId(0);
    m_nodeManager->setRootNode(m_rootNode.get());

    // Insert bookmarks bar folder
    BookmarkNode *bookmarkBar = m_nodeManager->addFolder(QLatin1String("Bookmarks Bar"), m_rootNode.get());

    // Set root node again so the node manager will fetch the bookmark bar
    m_nodeManager->setRootNode(m_rootNode.get());

    // Insert bookmark for search engine
    m_nodeManager->appendBookmark(QLatin1String("Search Engine"), QUrl(QLatin1String("https://www.startpage.com")), bookmarkBar);
}

 void BookmarkManager::save()
 {

 }

void BookmarkManager::load()
{
    // Check if table structure needs update before loading
    QSqlQuery query(m_database);
    if (query.exec(QLatin1String("PRAGMA table_info(Bookmarks)")))
    {
        bool hasSortcutColumn = false;
        QString shortcutColumn("Shortcut");

        while (query.next())
        {
            QString columnName = query.value(1).toString();
            if (columnName.compare(shortcutColumn) == 0)
                hasSortcutColumn = true;
        }

        if (!hasSortcutColumn)
        {
            if (!query.exec(QLatin1String("ALTER TABLE Bookmarks ADD Shortcut TEXT")))
                qDebug() << "Error updating bookmark table with shortcut column";
        }
    }

    // Don't load twice
    if (m_rootNode->getNumChildren() == 0)
    {
        m_rootNode->setFolderId(0);
        m_rootNode->setUniqueId(0);

        loadFolder(m_rootNode.get());

        m_nodeManager->setRootNode(m_rootNode.get());
    }
}
