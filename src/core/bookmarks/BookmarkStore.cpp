#include "BookmarkNode.h"
#include "BookmarkStore.h"

#include <deque>
#include <iterator>
#include <cstdint>

#include <QDebug>

BookmarkStore::BookmarkStore(const QString &databaseFile) :
    DatabaseWorker(databaseFile),
    m_rootNode(std::make_shared<BookmarkNode>(BookmarkNode::Folder, QLatin1String("Bookmarks")))
{
}

BookmarkStore::~BookmarkStore()
{
    save();
}

std::shared_ptr<BookmarkNode> BookmarkStore::getRootNode() const
{
    return m_rootNode;
}

int BookmarkStore::getMaxUniqueId() const
{
    int lastUniqueId = -1;

    auto stmt = m_database.prepare(R"(SELECT MAX(ID) FROM Bookmarks)");
    if (stmt.next())
        stmt >> lastUniqueId;

    stmt = m_database.prepare(R"(SELECT MAX(ParentID) FROM Bookmarks)");
    if (stmt.next())
    {
        int id = -1;
        stmt >> id;
        lastUniqueId = std::max(lastUniqueId, id);
    }

    return std::max(0, lastUniqueId);
}

void BookmarkStore::insertNode(int nodeId, int parentId, int nodeType, const QString &name, const QUrl &url, int position)
{
    auto stmt = m_database.prepare(R"(INSERT OR REPLACE INTO Bookmarks(ID, ParentID, Type, Name, URL, Position) VALUES (?, ?, ?, ?, ?, ?))");
    stmt << nodeId
         << parentId
         << nodeType
         << name
         << url
         << position;

    if (!stmt.execute())
        qWarning() << "BookmarkStore::onBookmarkCreated - could not create bookmark node.";

    stmt = m_database.prepare(R"(UPDATE Bookmarks SET Position = Position + 1 WHERE ParentID = ? AND Position >= ?)");
    stmt << parentId
         << position;

    if (!stmt.execute())
        qWarning() << "BookmarkStore::onBookmarkCreated - could not update bookmark positions.";
}

void BookmarkStore::removeNode(int nodeId, int parentId, int position)
{
    auto stmt = m_database.prepare(R"(DELETE FROM Bookmarks WHERE ID = ? OR ParentID = ?)");
    stmt << nodeId
         << nodeId;
    if (!stmt.execute())
        qWarning() << "BookmarkStore::onBookmarkDeleted - could not delete bookmark node.";

    // Update positions
    stmt = m_database.prepare(R"(UPDATE Bookmarks SET Position = Position - 1 WHERE ParentID = ? AND Position >= ?)");
    stmt << parentId
         << position;
    if (!stmt.execute())
        qWarning() << "BookmarkStore::onBookmarkDeleted - could not update bookmark positions";
}

void BookmarkStore::updateNode(int nodeId, int parentId, const QString &name, const QUrl &url, const QString &shortcut, int position)
{
    auto stmt =
            m_database.prepare(R"(UPDATE Bookmarks SET ParentID = ?, Name = ?, URL = ?, Shortcut = ?, Position = ? WHERE ID = ?)");
    stmt << parentId
         << name
         << url
         << shortcut
         << position
         << nodeId;

    if (!stmt.execute())
        qWarning() << "BookmarkStore::onBookmarkChanged - could not update bookmark node.";

    stmt = m_database.prepare(R"(UPDATE Bookmarks SET Position = Position + 1 WHERE ParentID = ? AND Position >= ? AND ID != ?)");
    stmt << parentId
         << position
         << nodeId;

    if (!stmt.execute())
        qWarning() << "BookmarkStore::onBookmarkChanged - could not update bookmark positions.";
}

void BookmarkStore::loadFolder(BookmarkNode *folder)
{
    if (!folder)
    {
        qDebug() << "Error: null pointer passed to BookmarkStore::loadFolder";
        return;
    }

    auto stmt = m_database.prepare(R"(SELECT ID, Type, Name, URL, Shortcut FROM Bookmarks WHERE ParentID = ? ORDER BY Position ASC)");

    // Iteratively load folder and all of its subfolders
    std::deque<BookmarkNode*> subFolders;
    subFolders.push_back(folder);
    while (!subFolders.empty())
    {
        BookmarkNode *n = subFolders.front();
        stmt << n->getUniqueId();
        if (!stmt.execute())
        {
            qWarning() << "Error loading bookmarks for folder " << n->getName();
            subFolders.pop_front();
            continue;
        }

        while (stmt.next())
        {
            int uniqueId = 0, nodeTypeInt = 0;
            QString name;
            QUrl url;
            QString shortcut;
            stmt >> uniqueId
                 >> nodeTypeInt
                 >> name
                 >> url
                 >> shortcut;

            BookmarkNode::NodeType nodeType = static_cast<BookmarkNode::NodeType>(nodeTypeInt);
            BookmarkNode *subNode = n->appendNode(std::make_unique<BookmarkNode>(nodeType, name));
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
                {
                    subNode->setURL(url);
                    subNode->setShortcut(shortcut);
                    break;
                }
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
    if (!m_database.execute(
                "CREATE TABLE IF NOT EXISTS Bookmarks(ID INTEGER PRIMARY KEY, ParentID INTEGER DEFAULT 0, "
                "Type INTEGER DEFAULT 0, Name TEXT, URL TEXT, Shortcut TEXT, Position INTEGER DEFAULT 0)"))
        qWarning() << "Error creating table Bookmarks. Message: " << QString::fromStdString(m_database.getLastError());

    // Insert root bookmark folder
    auto stmt = m_database.prepare(R"(INSERT INTO Bookmarks(ID, ParentID, Type, Name) VALUES (?, ?, ?, ?))");
    stmt.bind(0, 0);
    stmt.bind(1, -1);
    stmt.bind(2, static_cast<int>(BookmarkNode::Folder));
    stmt.bind(3, "Bookmarks");
    if (!stmt.execute())
        qDebug() << "Error inserting root bookmark folder.";

    m_rootNode->setUniqueId(0);

    BookmarkNode *bookmarkBar = m_rootNode->appendNode(std::make_unique<BookmarkNode>(BookmarkNode::Folder, QLatin1String("Bookmarks Bar")));
    bookmarkBar->setUniqueId(1);
    insertNode(1, 0, static_cast<int>(BookmarkNode::Folder), bookmarkBar->getName(), bookmarkBar->getURL(), 0);

    BookmarkNode *bookmark = bookmarkBar->appendNode(std::make_unique<BookmarkNode>(BookmarkNode::Bookmark, QLatin1String("Search Engine")));
    bookmark->setURL(QUrl(QLatin1String("https://www.startpage.com")));
    bookmark->setUniqueId(2);
    insertNode(2, 1, static_cast<int>(BookmarkNode::Bookmark), bookmark->getName(), bookmark->getURL(), 0);
}

void BookmarkStore::save()
{
    if (!m_database.beginTransaction())
    {
        qWarning() << "BookmarkStore::save - could not start transaction";
        return;
    }

    if (!m_database.execute("DELETE FROM Bookmarks"))
        qWarning() << "BookmarkStore::save - Could not clear bookmarks table";

    auto stmt = m_database.prepare(R"(INSERT INTO Bookmarks(ID, ParentID, Type, Name, URL, Shortcut, Position) VALUES (?, ?, ?, ?, ?, ?, ?))");

    auto saveNode = [&stmt](BookmarkNode *node) {
        stmt << *node;
        if (!stmt.execute())
            qWarning() << "Could not save bookmark " << node->getName() << ", id " << node->getUniqueId();
        stmt.reset();
    };

    std::deque<BookmarkNode*> queue;
    queue.push_back(m_rootNode.get());
    while (!queue.empty())
    {
        BookmarkNode *node = queue.back();
        saveNode(node);
        queue.pop_back();

        for (auto &child : node->m_children)
        {
            if (child->getType() == BookmarkNode::Folder)
                queue.push_back(child.get());
            else
                saveNode(child.get());
        }
    }

    if (!m_database.commitTransaction())
        qWarning() << "BookmarkStore::save - could not commit transaction";
}

void BookmarkStore::load()
{
    // Check if table structure needs update before loading
    auto stmt = m_database.prepare("PRAGMA table_info(Bookmarks)");
    if (stmt.execute())
    {
        bool hasSortcutColumn = false;
        const QString shortcutColumn("Shortcut");

        bool hasFolderIdColumn = false;
        const QString folderIdColumn("FolderID");

        while (stmt.next())
        {
            int cid = 0;
            QString columnName;
            stmt >> cid
                 >> columnName;
            if (columnName.compare(shortcutColumn) == 0)
                hasSortcutColumn = true;
            else if (columnName.compare(folderIdColumn) == 0)
                hasFolderIdColumn = true;
        }

        if (!hasSortcutColumn)
        {
            if (!m_database.execute("ALTER TABLE Bookmarks ADD Shortcut TEXT"))
                qDebug() << "Error updating bookmark table with shortcut column";
        }

        if (hasFolderIdColumn)
        {
            if (!m_database.beginTransaction())
                qWarning() << "BookmarkStore::load - could not start transaction";

            if (!m_database.execute("CREATE TABLE Bookmarks_New(ID INTEGER PRIMARY KEY, ParentID INTEGER DEFAULT 0, "
                                          "Type INTEGER DEFAULT 0, Name TEXT, URL TEXT, Shortcut TEXT, Position INTEGER DEFAULT 0)"))
                qWarning() << "BookmarkStore::load - could not update table structure";

            if (!m_database.execute("INSERT INTO Bookmarks_New SELECT ID, ParentID, Type, Name, URL, Shortcut, Position FROM Bookmarks"))
                qWarning() << "BookmarkStore::load - could not migrate bookmark data";

            if (!m_database.execute("DROP TABLE Bookmarks"))
                qWarning() << "BookmarkStore::load - could not migrate bookmark data";

            if (!m_database.execute("ALTER TABLE Bookmarks_New RENAME TO Bookmarks"))
                qWarning() << "BookmarkStore::load - could not migrate bookmark data";

            if (!m_database.commitTransaction())
                qWarning() << "BookmarkStore::load - could not commit transaction";
        }
    }

    // Don't load twice
    if (m_rootNode->getNumChildren() == 0)
    {
        m_rootNode->setUniqueId(0);
        loadFolder(m_rootNode.get());
    }
}
