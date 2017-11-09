#ifndef BOOKMARKMANAGER_H
#define BOOKMARKMANAGER_H

#include "DatabaseWorker.h"
#include <QString>
#include <list>

/// Individual bookmark structure
struct Bookmark
{
    /// Name to display for the bookmark
    QString name;

    /// Location of the bookmark
    QString URL;

    /// Position of the bookmark
    int position;

    /// Default constructor
    Bookmark() = default;

    /// Construct bookmark with name and url
    Bookmark(QString Name, QString Url) : name(Name), URL(Url) {}
};

/// Folder structure containing bookmarks and other folders
struct BookmarkFolder
{
    /// Unique ID of the folder
    int id;

    /// Name of the folder
    QString name;

    /// Position of the folder in bookmarks tree
    int treePosition;

    /// List of sub-folders
    std::list<BookmarkFolder*> folders;

    /// List of bookmarks belonging to the folder
    std::list<Bookmark*> bookmarks;

    /// Pointer to the folder's parent
    BookmarkFolder *parent;

    /// Default constructor
    BookmarkFolder() = default;
};

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

    /// Destructor - deallocates resources
    ~BookmarkManager();

    /// Returns the root bookmark folder
    BookmarkFolder *getRoot();

    /**
     * @brief addFolder Adds a folder to the bookmark collection, given a folder name and parent identifier
     * @param name Name of the folder to be created
     * @param parentID Identifier of the parent folder
     * @return The identifier of the newly created bookmark folder, or -1 if there was an error
     */
    int addFolder(const QString &name, int parentID);

    /**
     * @brief addBookmark Adds a bookmark to the collection
     * @param name Name to display as a reference to the bookmark
     * @param url Location of the bookmark (ex: http://xyz.co/page123)
     * @param folderID Optional identifier of the parent folder to place the bookmark into. Defaults to root folder
     */
    void addBookmark(const QString &name, const QString &url, int folderID = 0);

    /**
     * @brief addBookmark Adds a bookmark to the collection
     * @param name Name to display as a reference to the bookmark
     * @param url Location of the bookmark
     * @param folder Pointer to the folder the bookmark will belong to
     * @param position Optional position specification of the new bookmark
     */
    void addBookmark(const QString &name, const QString &url, BookmarkFolder *folder, int position = -1);

    /// Checks if the given url is bookmarked, returning true if it is
    bool isBookmarked(const QString &url);

    /// Removes the bookmark with the given URL (if it is a bookmark) from storage, returning true on success
    bool removeBookmark(const QString &url);

    /// Removes the given bookmark from storage
    void removeBookmark(Bookmark *item, BookmarkFolder *parent);

    /// Removes the given folder from storage, along with the bookmarks and sub-folders belonging to it
    void removeFolder(BookmarkFolder *folder);

    /// Sets the relative position of the given bookmark item in relation to the other bookmarks of its folder
    void setBookmarkPosition(Bookmark *item, BookmarkFolder *parent, int position);

    //todo: following method
    //void setFolderPosition(int folderID, int position);

protected:
    /// Called by the BookmarkTableModel when the URL and/or name of a bookmark has been modified
    void updatedBookmark(Bookmark *bookmark, Bookmark oldValue, int folderID);

    /// Called by the BookmarkFolderModel when the name of a folder has been changed
    void updatedFolderName(BookmarkFolder *folder);

private:
    /// Returns true if the given folder id exists in the bookmarks database, false if else
    bool isValidFolderID(int id);

    /// Loads bookmark information from the database
    void loadFolder(BookmarkFolder *folder);

    /// Searches for the folder with the given ID by performing a BFS from the root bookmark node
    /// Returns a nullptr if the folder cannot be found, otherwise returns a pointer to the folder
    BookmarkFolder *findFolder(int id);

    /**
     * @brief addBookmarkToDB Inserts a new bookmark into the database
     * @param bookmark Pointer to the bookmark
     * @param folder Folder the bookmark will belong to
     * @return True on successful insertion, false if otherwise
     */
    bool addBookmarkToDB(Bookmark *bookmark, BookmarkFolder *folder);

    /**
     * @brief removeBookmarkFromDB Removes an existing bookmark from the database
     * @param bookmark Pointer to the bookmark
     * @param folderId Unique Identifier of the bookmark's parent folder
     * @return True on success, false on failure
     */
    bool removeBookmarkFromDB(Bookmark *bookmark, int folderId);

    /**
     * @brief getNextBookmarkPos Calculates the next logical bookmark position for the folder
     * @param folder Pointer to the folder
     * @return The next position of a bookmark to be appended to the folder
     */
    int getNextBookmarkPos(BookmarkFolder *folder);

protected:
    /// Creates the initial table structures and default bookmarks if necessary
    void setup() override;

    /// Not used - values are saved to DB as soon as they are changed by the user
    void save() override {}

    /// Loads bookmarks from the database
    void load() override;

protected:
    /// Root bookmark folder
    BookmarkFolder m_root;
};

#endif // BOOKMARKMANAGER_H
