#ifndef BOOKMARKMANAGER_H
#define BOOKMARKMANAGER_H

#include "DatabaseWorker.h"
#include <QString>
#include <list>
#include <memory>

class BookmarkNode;

/**
 * @class BookmarkManager
 * @brief Loads bookmark information from the database and acts as
 *        an interface for the user to view or modify their bookmark
 *        collection
 */
class BookmarkManager : private DatabaseWorker
{
    friend class BookmarkTableModel;
    friend class BookmarkFolderModel;
    friend class DatabaseWorker;

public:
    /// Bookmark constructor - loads database information into memory
    explicit BookmarkManager(const QString &databaseFile);

    /// Returns the root bookmark container
    BookmarkNode *getRoot();

    /// Returns the bookmarks bar
    BookmarkNode *getBookmarksBar();

    /**
     * @brief addFolder Adds a folder to the bookmark collection, given a folder name and parent identifier
     * @param name Name of the folder to be created
     * @param parent Pointer to the parent folder
     * @return A pointer to the new folder
     */
    BookmarkNode *addFolder(const QString &name, BookmarkNode *parent);

    /**
     * @brief appendBookmark Adds a bookmark to the collection, at the end of its parent folder
     * @param name Name to display as a reference to the bookmark
     * @param url Location of the bookmark (ex: http://xyz.co/page123)
     * @param folder Optional pointer to the parent folder. Defaults to root folder if not given.
     */
    void appendBookmark(const QString &name, const QString &url, BookmarkNode *folder = nullptr);

    /**
     * @brief insertBookmark Adds a bookmark to the collection at a specific position
     * @param name Name to display as a reference to the bookmark
     * @param url Location of the bookmark
     * @param folder Pointer to the parent folder that the bookmark will belong to
     * @param position Relative position of the new bookmark
     */
    void insertBookmark(const QString &name, const QString &url, BookmarkNode *folder, int position);

    /// Checks if the given url is bookmarked, returning true if it is
    bool isBookmarked(const QString &url);

    /// Removes the bookmark with the given URL (if it is a bookmark) from storage
    void removeBookmark(const QString &url);

    /// Removes the given bookmark from storage
    void removeBookmark(BookmarkNode *item);

    /// Removes the given folder from storage, along with the bookmarks and sub-folders belonging to it
    void removeFolder(BookmarkNode *folder);

    /// Sets the position of the given node, relative to other child nodes of the same parent
    void setNodePosition(BookmarkNode *node, int position);

    /**
     * @brief Changes the parent node of the given folder to the new parent
     * @param folder Folder that is being assigned to a new parent
     * @param newParent The folder that will become the new parent of the node
     * @returns An updated pointer to the folder that was moved
     */
    BookmarkNode *setFolderParent(BookmarkNode *folder, BookmarkNode *newParent);

protected:
    /// Called by the BookmarkTableModel when the URL and/or name of a bookmark has been modified
    void updatedBookmark(BookmarkNode *bookmark, BookmarkNode &oldValue, int folderID);

    /// Called by the BookmarkFolderModel when the name of a folder has been changed
    void updatedFolderName(BookmarkNode *folder);

private:
    /// Returns true if the given folder id exists in the bookmarks database, false if else
    bool isValidFolderID(int id);

    /// Loads bookmark information from the database
    void loadFolder(BookmarkNode *folder);

    /// Searches for the folder with the given ID by performing a BFS from the root bookmark node
    /// Returns a nullptr if the folder cannot be found, otherwise returns a pointer to the folder
    BookmarkNode *findFolder(int id);

    /**
     * @brief addBookmarkToDB Inserts a new bookmark into the database
     * @param bookmark Pointer to the bookmark
     * @param folder Folder the bookmark will belong to
     * @return True on successful insertion, false if otherwise
     */
    bool addBookmarkToDB(BookmarkNode *bookmark, BookmarkNode *folder);

    /**
     * @brief removeBookmarkFromDB Removes an existing bookmark from the database
     * @param bookmark Pointer to the bookmark
     * @return True on success, false on failure
     */
    bool removeBookmarkFromDB(BookmarkNode *bookmark);

protected:
    /// Creates the initial table structures and default bookmarks if necessary
    void setup() override;

    /// Not used - values are saved to DB as soon as they are changed by the user
    void save() override {}

    /// Loads bookmarks from the database
    void load() override;

protected:
    /// Root bookmark folder
    std::unique_ptr<BookmarkNode> m_rootNode;
};

#endif // BOOKMARKMANAGER_H
