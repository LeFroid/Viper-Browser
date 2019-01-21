#ifndef BOOKMARKMANAGER_H
#define BOOKMARKMANAGER_H

#include "DatabaseWorker.h"
#include "LRUCache.h"

#include <map>
#include <memory>
#include <string>
#include <vector>

#include <QObject>
#include <QSqlQuery>
#include <QString>

class BookmarkNode;

/**
 * @defgroup Bookmarks Bookmark System
 */

/**
 * @class BookmarkManager
 * @brief Loads bookmark information from the database and acts as
 *        an interface for the user to view or modify their bookmark
 *        collection
 * @ingroup Bookmarks
 */
class BookmarkManager : public QObject, private DatabaseWorker
{
    Q_OBJECT

    friend class BookmarkImporter;
    friend class BookmarkTableModel;
    friend class BookmarkFolderModel;
    friend class DatabaseFactory;

public:
    typedef typename std::vector<BookmarkNode*>::iterator iterator;
    typedef typename std::vector<BookmarkNode*>::const_iterator const_iterator;

    /// Bookmark constructor - loads database information into memory
    explicit BookmarkManager(const QString &databaseFile, QObject *parent = nullptr);

    ~BookmarkManager();

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
    void appendBookmark(const QString &name, const QUrl &url, BookmarkNode *folder = nullptr);

    /**
     * @brief insertBookmark Adds a bookmark to the collection at a specific position
     * @param name Name to display as a reference to the bookmark
     * @param url Location of the bookmark
     * @param folder Pointer to the parent folder that the bookmark will belong to
     * @param position Relative position of the new bookmark
     */
    void insertBookmark(const QString &name, const QUrl &url, BookmarkNode *folder, int position);

    /// Checks if the given url is bookmarked, returning true if it is
    bool isBookmarked(const QUrl &url);

    /// Removes the bookmark with the given URL (if it is a bookmark) from storage
    void removeBookmark(const QUrl &url);

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

    /**
     * @brief Searches for a bookmark that is assigned the given URL
     * @param url URL of the bookmark node
     * @return A pointer to the bookmark node if found, otherwise returns a nullptr
     */
    BookmarkNode *getBookmark(const QUrl &url);

    /// Updates the name of a bookmark in the database
    void updateBookmarkName(const QString &name, BookmarkNode *bookmark);

    /// Updates the shortcut for a bookmark in the database
    void updateBookmarkShortcut(const QString &shortcut, BookmarkNode *bookmark);

    /// Updates the URL of a bookmark in the database
    void updateBookmarkURL(const QUrl &url, BookmarkNode *bookmark);

    /// Called by the BookmarkFolderModel when the name of a folder has been changed
    void updatedFolderName(BookmarkNode *folder);

    /// Returns an iterator pointing to the first bookmark in the collection
    iterator begin() { return m_nodeList.begin(); }

    /// Returns an iterator at the end of the bookmark collection
    iterator end() { return m_nodeList.end(); }

signals:
    /// Emitted when there has been a change to the bookmark tree
    void bookmarksChanged();

private:
    /// Loads bookmark information from the database
    void loadFolder(BookmarkNode *folder);

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

    /// Called when the bookmark tree has changed - resets the bookmark list for iteration, and emits the bookmarksChanged() signal
    void onBookmarksChanged();

    /// Resets the flat list of bookmark node pointers, used for iteration & bookmark searches
    void resetBookmarkList();

protected:
    /// Lets the bookmark manager know an import has started or finished, so resetBookmarkList() won't be called until the last bookmark has been imported
    void setImportState(bool val);

    /// Returns true if the bookmark database contains the table structure(s) needed for it to function properly,
    /// false if else.
    bool hasProperStructure() override;

    /// Creates the initial table structures and default bookmarks if necessary
    void setup() override;

    /// Not used - values are saved to DB as soon as they are changed by the user
    void save() override {}

    /// Loads bookmarks from the database
    void load() override;

private:
    /// Root bookmark folder
    std::unique_ptr<BookmarkNode> m_rootNode;

    /// Container of bookmark node pointers, flattened version of tree structure used for bookmark iteration
    std::vector<BookmarkNode*> m_nodeList;

    /// Bookmark import state - true if bookmarks being imported, false if else
    bool m_importState;

    /// Cache of bookmark nodes that were recently searched for within the application
    LRUCache<std::string, BookmarkNode*> m_lookupCache;
};

#endif // BOOKMARKMANAGER_H
