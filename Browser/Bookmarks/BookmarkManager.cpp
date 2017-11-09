#include "BookmarkManager.h"

#include <deque>
#include <iterator>
#include <cstdint>
#include <QSqlError>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QDebug>

BookmarkManager::BookmarkManager(const QString &databaseFile) :
    DatabaseWorker(databaseFile, "Bookmarks")
{
    m_root.id = 0;
    m_root.parent = nullptr;
    m_root.name = "Bookmarks";
}

BookmarkManager::~BookmarkManager()
{
    // remove folders and bookmarks from heap
    std::deque<BookmarkFolder*> folderQ;

    for (BookmarkFolder *b : m_root.folders)
        folderQ.push_back(b);

    for (Bookmark *b : m_root.bookmarks)
        delete b;

    while(!folderQ.empty())
    {
        BookmarkFolder *folder = folderQ.front();
        for (BookmarkFolder *b : folder->folders)
            folderQ.push_back(b);
        for (Bookmark *b : folder->bookmarks)
            delete b;
        folderQ.pop_front();
        delete folder;
    }
}

BookmarkFolder *BookmarkManager::getRoot()
{
    return &m_root;
}

int BookmarkManager::addFolder(const QString &name, int parentID)
{
    // Search for folder with given parent ID
    BookmarkFolder *parent = findFolder(parentID);

    // If parent cannot be found then attach the new folder directly to the root folder
    if (!parent)
    {
        qDebug() << "[Warning]: BookmarkManager::addFolder(..) - Could not find parent folder for new folder \"" << name
                 << "\", setting parent folder to root. ";
        parent = &m_root;
    }

    // Set position of folder to the bottom of its parent folder
    int position = parent->bookmarks.size() + parent->folders.size();

    // Insert folder into DB then append to parent folder's list of subfolders
    QSqlQuery query(m_database);
    query.prepare("INSERT OR IGNORE INTO BookmarkFolders(ParentID, Name, Position) VALUES (:parentID, :name, :position)");
    query.bindValue(":parentID", parent->id);
    query.bindValue(":name", name);
    query.bindValue(":position", position);
    if (!query.exec())
        qDebug() << "[Error]: In BookmarkManager::addFolder(..) - error inserting new bookmark folder into database. Message: " << query.lastError().text();

    // Fetch new folder's id from DB
    int newFolderID = -1;
    query.prepare("SELECT FolderID from BookmarkFolders WHERE ParentID = (:id) AND Name = (:name)");
    query.bindValue(":id", parent->id);
    query.bindValue(":name", name);
    if (query.exec() && query.next())
        newFolderID = query.value(0).toInt();

    // Append bookmark folder to parent's folder container
    BookmarkFolder *f = new BookmarkFolder;
    f->id = newFolderID;
    f->treePosition = parent->folders.size();
    f->name = name;
    f->parent = parent;
    parent->folders.push_back(f);

    return f->id;
}

void BookmarkManager::addBookmark(const QString &name, const QString &url, int folderID)
{
    // Create new bookmark
    Bookmark *b = new Bookmark(name, url);

    // Find parent folder, and if cannot be found, set to root folder
    BookmarkFolder *parent = findFolder(folderID);
    if (!parent)
    {
        qDebug() << "[Warning]: BookmarkManager::addBookmark(..) - Could not find parent folder for new bookmark \"" << name
                 << "\", setting parent folder to root. ";
        parent = &m_root;
    }

    // Calculate position of new bookmark
    b->position = getNextBookmarkPos(parent);
    if (b->position < 0)
        b->position = parent->bookmarks.size() + parent->folders.size();

    // Add bookmark and then update the database
    parent->bookmarks.push_back(b);

    if (!addBookmarkToDB(b, parent))
        qDebug() << "[Warning]: Could not insert new bookmark into the database";
}

void BookmarkManager::addBookmark(const QString &name, const QString &url, BookmarkFolder *folder, int position)
{
    if (!folder)
        return;

    // Create new bookmark
    Bookmark *b = new Bookmark(name, url);

    // Calculate default position of new bookmark
    bool isCustomPos = (position > -1 && uint64_t(position) < folder->bookmarks.size());
    b->position = getNextBookmarkPos(folder);
    if (b->position < 0)
        b->position = folder->bookmarks.size() + folder->folders.size();

    if (!addBookmarkToDB(b, folder))
        qDebug() << "[Warning]: Could not insert new bookmark into the database";

    folder->bookmarks.push_back(b);

    if (isCustomPos && position >= 0)
        setBookmarkPosition(b, folder, position);
}

bool BookmarkManager::isBookmarked(const QString &url)
{
    QSqlQuery query(m_database);
    query.prepare("SELECT FolderID FROM Bookmarks WHERE URL = (:url)");
    query.bindValue(":url", url);
    return (query.exec() && query.next());
}

bool BookmarkManager::removeBookmark(const QString &url)
{
    // Search DB rather than traverse tree
    QSqlQuery query(m_database);
    query.prepare("SELECT FolderID FROM Bookmarks WHERE URL = (:url)");
    query.bindValue(":url", url);
    if (!query.exec() || !query.first())
    {
        qDebug() << "[Warning]: In BookmarkManager::removeBookmark(..) - could not search database for bookmark with url " << url;
        return false;
    }
    int folderId = query.value(0).toInt();
    BookmarkFolder *folder = findFolder(folderId);
    if (!folder)
    {
        qDebug() << "[Warning]: In BookmarkManager::removeBookmark(..) - found bookmark, could not find folder it belongs to";
        return false;
    }
    Bookmark *bookmark = nullptr;
    for (Bookmark *b : folder->bookmarks)
    {
        if (b->URL.compare(url, Qt::CaseInsensitive) == 0)
        {
            bookmark = b;
            folder->bookmarks.remove(b);
            break;
        }
    }
    if (!bookmark)
        return false;

    if (!removeBookmarkFromDB(bookmark, folderId))
        qDebug() << "Could not remove bookmark from DB";

    delete bookmark;
    return true;
}

void BookmarkManager::removeBookmark(Bookmark *item, BookmarkFolder *parent)
{
    if (!item || !parent)
        return;

    // Search for bookmark
    auto it = std::find(parent->bookmarks.begin(), parent->bookmarks.end(), item);
    if (it == parent->bookmarks.end())
    {
        qDebug() << "[Warning]: Invalid bookmark given in BookmarkManager::removeBookmark. Does not belong to parent folder.";
        return;
    }

    // Remove from DB, then from memory
    if (!removeBookmarkFromDB(item, parent->id))
        qDebug() << "Could not remove bookmark from DB";

    parent->bookmarks.erase(it);
    delete item;
}

void BookmarkManager::removeFolder(BookmarkFolder *folder)
{
    if (!folder || folder->id == 0)
        return;

    // Delete all bookmarks belonging to folder from the database
    QSqlQuery query(m_database);
    query.prepare("DELETE FROM Bookmarks WHERE FolderID = (:id)");
    query.bindValue(":id", folder->id);
    if (!query.exec())
    {
        qDebug() << "[Warning]: Could not remove bookmarks from database in BookmarkManager::removeFolder. Message: "
                 << query.lastError().text();
    }

    // Remove folder's bookmarks from memory
    for (Bookmark *bookmark : folder->bookmarks)
        delete bookmark;

    // Delete folder from database
    query.prepare("DELETE FROM BookmarkFolders WHERE FolderID = (:id)");
    query.bindValue(":id", folder->id);
    if (!query.exec())
    {
        qDebug() << "[Warning]: In BookmarkManager::removeFolder - could not remove bookmark folder "
                 << folder->name << " from database. Message: " << query.lastError().text();
    }

    // Recursively remove sub-folder data
    for (BookmarkFolder *subFolder : folder->folders)
        removeFolder(subFolder);

    // Remove folder from parent
    if (folder->parent)
        folder->parent->folders.remove(folder);

    // Remove folder from memory
    delete folder;
}

void BookmarkManager::setBookmarkPosition(Bookmark *item, BookmarkFolder *parent, int position)
{
    if (!item || !parent)
        return;

    // Make sure the position is valid
    if (position < 0 || uint64_t(position) >= parent->bookmarks.size() + parent->folders.size())
        return;

    // Ensure bookmark belongs to the folder
    auto it = std::find(parent->bookmarks.begin(), parent->bookmarks.end(), item);
    if (it == parent->bookmarks.end() || item->position == position)
        return;

    // Update database
    QSqlQuery queryBookmarks(m_database), queryFolders(m_database);

    // If position is being shifted up (ie new index less than current), increment position of items between old and new positions.
    if (item->position > position)
    {
        queryBookmarks.prepare("UPDATE Bookmarks SET Position = Position + 1 WHERE FolderID = (:folderId) AND Position >= (:posNew) AND Position < (:posOld)");
        queryFolders.prepare("UPDATE BookmarkFolders SET Position = Position + 1 WHERE ParentID = (:folderId) AND Position >= (:posNew) AND Position < (:posOld)");
    }
    else // If position is being shifted down, decrement position of items between old and new positions
    {
        queryBookmarks.prepare("UPDATE Bookmarks SET Position = Position - 1 WHERE FolderID = (:folderId) AND Position <= (:posNew) AND Position > (:posOld)");
        queryFolders.prepare("UPDATE BookmarkFolders SET Position = Position - 1 WHERE ParentID = (:folderId) AND Position <= (:posNew) AND Position > (:posOld)");
    }
    queryBookmarks.bindValue(":folderId", parent->id);
    queryBookmarks.bindValue(":posNew", position);
    queryBookmarks.bindValue(":posOld", item->position);
    if (!queryBookmarks.exec())
        qDebug() << "[Warning]: In BookmarkManager::setBookmarkPosition - could not update positions of bookmarks. "
                 << "Message: " << queryBookmarks.lastError().text();
    queryFolders.bindValue(":folderId", parent->id);
    queryFolders.bindValue(":posNew", position);
    queryFolders.bindValue(":posOld", item->position);
    if (!queryFolders.exec())
        qDebug() << "[Warning]: In BookmarkManager::setBookmarkPosition - could not update positions of folders. "
                 << "Message: " << queryFolders.lastError().text();

    // Adjust position of bookmark itself
    queryBookmarks.prepare("UPDATE Bookmarks SET Position = (:newPos) WHERE URL = (:url)");
    queryBookmarks.bindValue(":newPos", position);
    queryBookmarks.bindValue(":url", item->URL);
    if (!queryBookmarks.exec())
        qDebug() << "[Warning]: In BookmarkManager::setBookmarkPosition - could not update position of bookmark. "
                 << "Message: " << queryBookmarks.lastError().text();

    // Adjust bookmark list for new positions
    queryBookmarks.prepare("SELECT * FROM Bookmarks WHERE FolderID = (:id) ORDER BY POSITION ASC");
    queryBookmarks.bindValue(":id", parent->id);
    if (queryBookmarks.exec())
    {
        QSqlRecord bRec = queryBookmarks.record();
        int idBUrl = bRec.indexOf("URL");
        int idBName = bRec.indexOf("Name");
        int idBPos = bRec.indexOf("Position");
        it = parent->bookmarks.begin();
        while (queryBookmarks.next() && it != parent->bookmarks.end())
        {
            Bookmark *bookmark = *it;
            bookmark->URL = queryBookmarks.value(idBUrl).toString();
            bookmark->name = queryBookmarks.value(idBName).toString();
            bookmark->position = queryBookmarks.value(idBPos).toInt();
            ++it;
        }
    }
}

void BookmarkManager::updatedBookmark(Bookmark *bookmark, Bookmark oldValue, int folderID)
{
    if (!bookmark)
        return;

    // Update database
    QSqlQuery query(m_database);
    if (oldValue.URL == bookmark->URL)
    {
        query.prepare("UPDATE Bookmarks SET Name = (:newName) WHERE URL = (:url)");
        query.bindValue(":newName", bookmark->name);
        query.bindValue(":url", bookmark->URL);
        if (!query.exec())
            qDebug() << "[Warning]: BookmarkManager::updatedBookmark(..) - Could not modify bookmark name. Error message: "
                     << query.lastError().text();
    }
    else
    {
        // If primary key has changed, remove the old record and insert the bookmark as a new one
        int position = 0;
        query.prepare("SELECT Position FROM Bookmarks WHERE URL = (:url)");
        query.bindValue(":url", oldValue.URL);
        if (query.exec() && query.next())
        {
            position = query.value(0).toInt();
        }
        else
            qDebug() << "[Warning]: BookmarkManager::updatedBookmark(..) - Could not fetch position of bookmark with "
                        "URL " << oldValue.URL << ". Error message: " << query.lastError().text();
        query.prepare("DELETE FROM Bookmarks WHERE URL = (:url)");
        query.bindValue(":url", oldValue.URL);
        query.exec();
        query.prepare("INSERT INTO Bookmarks(URL, FolderID, Name, Position) VALUES(:url, :folderID, :name, :position)");
        query.bindValue(":url", bookmark->URL);
        query.bindValue(":folderID", folderID);
        query.bindValue(":name", bookmark->name);
        query.bindValue(":position", position);
        if (!query.exec())
            qDebug() << "[Warning]: BookmarkManager::updatedBookmark(..) - Could not insert updated bookmark to database. "
                        "Error message: " << query.lastError().text();
    }
}

void BookmarkManager::updatedFolderName(BookmarkFolder *folder)
{
    if (!folder)
        return;
    if (folder->id < 0)
        return;

    // Update database
    QSqlQuery query(m_database);
    query.prepare("UPDATE BookmarkFolders SET Name = (:name) WHERE FolderID = (:id)");
    query.bindValue(":name", folder->name);
    query.bindValue(":id", folder->id);
    if (!query.exec())
        qDebug() << "Error updating name of bookmark folder in database. Message: " << query.lastError().text();
}

bool BookmarkManager::isValidFolderID(int id)
{
    QSqlQuery query(m_database);
    query.prepare("SELECT ParentID FROM BookmarkFolders WHERE FolderID = (:id)");
    query.bindValue(":id", id);
    return (query.exec() && query.last());
}

void BookmarkManager::loadFolder(BookmarkFolder *folder)
{
    if (!folder)
    {
        qDebug() << "Error: null pointer passed to BookmarkManager::loadFolder";
        return;
    }

    QSqlQuery queryBookmark(m_database);
    queryBookmark.prepare("SELECT * FROM Bookmarks WHERE FolderID = (:id) ORDER BY POSITION ASC");

    // Load subfolders
    QSqlQuery queryFolder(m_database);
    queryFolder.prepare("SELECT FolderID, Name, Position FROM BookmarkFolders WHERE ParentID = (:id) ORDER BY Position ASC");
    queryFolder.bindValue(":id", folder->id);
    if (!queryFolder.exec())
        qDebug() << "Error fetching bookmark folders. Message: " << queryFolder.lastError().text();
    else
    {
        QSqlRecord rec = queryFolder.record();
        int idFolder = rec.indexOf("FolderID");
        int idName = rec.indexOf("Name");
        while (queryFolder.next())
        {
            BookmarkFolder *subFolder = new BookmarkFolder;
            subFolder->id = queryFolder.value(idFolder).toInt();
            subFolder->name = queryFolder.value(idName).toString();
            subFolder->treePosition = folder->folders.size();
            subFolder->parent = folder;

            // Load folders belonging to sub-folder, followed by bookmarks
            loadFolder(subFolder);

            folder->folders.push_back(subFolder);
        }
    }

    // Load bookmarks
    //queryBookmark.prepare("SELECT * FROM Bookmarks WHERE FolderID = (:id) ORDER BY POSITION ASC");
    queryBookmark.bindValue(":id", folder->id);
    if (queryBookmark.exec())
    {
        QSqlRecord bRec = queryBookmark.record();
        int idBUrl = bRec.indexOf("URL");
        int idBName = bRec.indexOf("Name");
        int idBPos = bRec.indexOf("Position");
        while (queryBookmark.next())
        {
            Bookmark *bookmark = new Bookmark;
            bookmark->URL = queryBookmark.value(idBUrl).toString();
            bookmark->name = queryBookmark.value(idBName).toString();
            bookmark->position = queryBookmark.value(idBPos).toInt();
            folder->bookmarks.push_back(bookmark);
        }
    }
}

BookmarkFolder *BookmarkManager::findFolder(int id)
{
    // Check database to ensure folder id is valid before performing a BFS
    if (!isValidFolderID(id))
        return nullptr;

    BookmarkFolder *currNode = nullptr;
    std::deque<BookmarkFolder*> queue;
    queue.push_back(&m_root);

    // BFS until the folder with given ID is found, or all folders are searched
    while (!queue.empty())
    {
        currNode = queue.front();
        if (!currNode)
        {
            queue.pop_front();
            continue;
        }

        if (currNode->id == id)
            return currNode;

        for (BookmarkFolder *b : currNode->folders)
            queue.push_back(b);

        queue.pop_front();
    }

    return nullptr;
}

bool BookmarkManager::addBookmarkToDB(Bookmark *bookmark, BookmarkFolder *folder)
{
    QSqlQuery query(m_database);
    query.prepare("INSERT OR REPLACE INTO Bookmarks(URL, FolderID, Name, Position) VALUES(:url, :folderID, :name, :position)");
    query.bindValue(":url", bookmark->URL);
    query.bindValue(":folderID", folder->id);
    query.bindValue(":name", bookmark->name);
    query.bindValue(":position", bookmark->position);
    return query.exec();
}

bool BookmarkManager::removeBookmarkFromDB(Bookmark *bookmark, int folderId)
{
    bool ok = true;

    QSqlQuery query(m_database);
    // Remove from DB
    query.prepare("DELETE FROM Bookmarks WHERE URL = (:url)");
    query.bindValue(":url", bookmark->URL);
    if (!query.exec())
    {
        qDebug() << "[Warning]: In BookmarkManager::removeBookmarkFromDB(..) - DB Error: " << query.lastError().text();
        ok = false;
    }
    // Update bookmark positions
    query.prepare("UPDATE Bookmarks SET Position = Position - 1 WHERE FolderID = (:folderId) AND Position > (:position)");
    query.bindValue(":folderId", folderId);
    query.bindValue(":position", bookmark->position);
    if (!query.exec())
    {
        qDebug() << "[Warning]: In BookmarkManager::removeBookmarkFromDB(..) - could not update position of bookmarks. DB Error: "
                 << query.lastError().text();
        ok = false;
    }
    // Update folder positions
    query.prepare("UPDATE BookmarkFolders SET Position = Position - 1 WHERE ParentID = (:folderId) AND Position > (:position)");
    query.bindValue(":folderId", folderId);
    query.bindValue(":position", bookmark->position);
    if (!query.exec())
    {
        qDebug() << "[Warning]: In BookmarkManager::removeBookmarkFromDB(..) - could not update position of folders. DB Error: "
                 << query.lastError().text();
        ok = false;
    }
    return ok;
}

int BookmarkManager::getNextBookmarkPos(BookmarkFolder *folder)
{
    if (!folder)
        return -1;

    QSqlQuery query(m_database);
    query.prepare("SELECT MAX(Position) FROM Bookmarks WHERE FolderID = (:id)");
    query.bindValue(":id", folder->id);
    if (!query.exec() || !query.next())
        return -1;
    int maxBookmarkPos = query.value(0).toInt();
    query.prepare("SELECT Max(Position) FROM BookmarkFolders WHERE ParentID = (:id)");
    query.bindValue(":id", folder->id);
    if (!query.exec() || !query.next())
        return maxBookmarkPos + 1;
    return std::max(query.value(0).toInt(), maxBookmarkPos) + 1;
}

void BookmarkManager::setup()
{
    // Setup table structures
    QSqlQuery query(m_database);
    query.prepare("CREATE TABLE IF NOT EXISTS BookmarkFolders(FolderID INTEGER PRIMARY KEY, "
                  "ParentID INTEGER, Name TEXT, Position INTEGER, FOREIGN KEY(ParentID) REFERENCES BookmarkFolders(FolderID))");
    if (!query.exec())
        qDebug() << "Error creating table BookmarkFolders. Message: " << query.lastError().text();

    query.prepare("CREATE TABLE IF NOT EXISTS Bookmarks(URL TEXT PRIMARY KEY, FolderID INTEGER, Name TEXT, Position INTEGER, "
                  "FOREIGN KEY(FolderID) REFERENCES BookmarkFolders(FolderID))");
    if (!query.exec())
        qDebug() << "Error creating table Bookmarks. Message: " << query.lastError().text();

    // Insert root bookmark folder
    query.prepare("INSERT OR IGNORE INTO BookmarkFolders(FolderID, Name) VALUES(:id, :name)");
    query.bindValue(":id", 0);
    query.bindValue(":name", "Bookmarks");
    if (!query.exec())
        qDebug() << "Error inserting root bookmark folder. Message: " << query.lastError().text();

    // Insert bookmark for ixquick.com
    query.prepare("INSERT OR IGNORE INTO Bookmarks(URL, FolderID, Name, Position) VALUES(:url, :folderID, :name, :position)");
    query.bindValue(":url", "https://www.ixquick.com/");
    query.bindValue(":folderID", 0);
    query.bindValue(":name", "Search Engine");
    query.bindValue(":position", 0);
    if (!query.exec())
        qDebug() << "Error inserting bookmark. Message: " << query.lastError().text();
}

void BookmarkManager::load()
{
    loadFolder(&m_root);
}
