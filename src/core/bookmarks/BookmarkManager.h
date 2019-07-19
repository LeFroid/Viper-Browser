#ifndef BOOKMARKNODEMANAGER_H
#define BOOKMARKNODEMANAGER_H

#include "DatabaseTaskScheduler.h"
#include "LRUCache.h"
#include "ServiceLocator.h"

#include <atomic>
#include <mutex>
#include <string>
#include <vector>

#include <QFuture>
#include <QObject>

class BookmarkNode;
class BookmarkStore;
class FaviconManager;

/**
 * @class BookmarkManager
 * @brief Handles the in-memory bookmark collection, emitting a signal after any change
 *        to a bookmark occurs so the \ref BookmarkStore can save the state to the database.
 * @ingroup Bookmarks
 */
class BookmarkManager : public QObject
{
    friend class BookmarkImporter;
    friend class BookmarkStore;
    friend class BookmarkManagerTest;

    Q_OBJECT

public:
    using iterator = std::vector<BookmarkNode*>::iterator;
    using const_iterator = std::vector<BookmarkNode*>::const_iterator;

    /// Constructs the bookmark node manager, given the service locator, task scheduler and a pointer to the manager's parent
    explicit BookmarkManager(const ViperServiceLocator &serviceLocator, DatabaseTaskScheduler &taskScheduler, QObject *parent);

    /// BookmarkManager destructor
    ~BookmarkManager();

    /// Returns an iterator pointing to the first bookmark in the collection
    const_iterator begin() const { return m_nodeList.cbegin(); }

    /// Returns an iterator at the end of the bookmark collection
    const_iterator end() const { return m_nodeList.cend(); }

    /// Returns the root of the bookmark tree
    BookmarkNode *getRoot() const;

    /// Returns the folder that acts as the bookmarks bar
    BookmarkNode *getBookmarksBar() const;

    /**
     * @brief Searches for a bookmark that is assigned the given URL
     * @param url URL of the bookmark node
     * @return A pointer to the bookmark node if found, otherwise returns a nullptr
     */
    BookmarkNode *getBookmark(const QUrl &url);

    /// Checks if the given url is bookmarked, returning true if it is
    bool isBookmarked(const QUrl &url);

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

    /**
     * @brief addFolder Adds a folder to the bookmark collection, given a folder name and parent identifier
     * @param name Name of the folder to be created
     * @param parent Pointer to the parent folder
     * @return A pointer to the new folder
     */
    BookmarkNode *addFolder(const QString &name, BookmarkNode *parent);

    /// Removes the bookmark with the given URL (if it is a bookmark) from storage
    void removeBookmark(const QUrl &url);

    /// Removes the given bookmark from storage
    void removeBookmark(BookmarkNode *item);

    /// Sets the name of a bookmark in the database
    void setBookmarkName(BookmarkNode *bookmark, const QString &name);

    /// Sets the bookmark's parent to the given node, returning a pointer to the moved bookmark.
    BookmarkNode *setBookmarkParent(BookmarkNode *bookmark, BookmarkNode *parent);

    /// Sets the position of the bookmark to the given value
    void setBookmarkPosition(BookmarkNode *bookmark, int position);

    /// Sets the shortcut for a bookmark in the database
    void setBookmarkShortcut(BookmarkNode *bookmark, const QString &shortcut);

    /// Sets the URL of a bookmark in the database
    void setBookmarkURL(BookmarkNode *bookmark, const QUrl &url);

Q_SIGNALS:
    /// Emitted when one of the properties of the given bookmark has changed
    void bookmarkChanged(const BookmarkNode *node);

    /// Emitted when there has been a change to the bookmark tree that requires an update to the UI
    void bookmarksChanged();

    /// Emitted when the given bookmark has been added to the tree
    void bookmarkCreated(const BookmarkNode *node);

    /// Emitted when a bookmark with the given unique identifier has been deleted.
    /// This can either be a bookmark node or a folder
    void bookmarkDeleted(int uniqueId, int parentId, int position);

protected:
    /// Sets the root node of the bookmark tree - this is called by the \ref BookmarkStore after loading the data
    void setRootNode(std::shared_ptr<BookmarkNode> node);

    /// Runs resetBookmarkList concurrently as long as a major change isn't being made to the bookmark collection
    void scheduleResetList();

    /// Sets the flag indicating whether or not the flattened list of bookmarks should be updated
    void setCanUpdateList(bool value);

    /// Waits on any asynchronous resetBookmarkList() operations
    void waitToFinishList();

private Q_SLOTS:
    /// Runs on a regular interval until the root bookmark node has been populated
    void checkIfLoaded();

private:
    /// Schedules an create bookmark operation in the repository
    void scheduleBookmarkInsert(const BookmarkNode *node);

    /// Schedules a boookmark update to the database worker
    void scheduleBookmarkUpdate(const BookmarkNode *node);

    /// Resets the flat list of bookmark node pointers, used for iteration & bookmark searches
    void resetBookmarkList();

private:
    /// Reference to the task scheduler. Needed to queue work for the \ref BookmarkStore
    DatabaseTaskScheduler &m_taskScheduler;

    /// Holds a pointer to the root node of the bookmark tree
    std::shared_ptr<BookmarkNode> m_rootNode;

    /// Holds a pointer to the bookmark bar node in the bookmark tree
    BookmarkNode *m_bookmarkBar;

    /// Points to the bookmark repository
    BookmarkStore *m_bookmarkStore;

    /// Pointer to the favicon manager
    FaviconManager *m_faviconManager;

    /// Cache of bookmark nodes that were recently searched for within the application
    LRUCache<std::string, BookmarkNode*> m_lookupCache;

    /// Container of bookmark node pointers, flattened version of tree structure used for bookmark iteration
    std::vector<BookmarkNode*> m_nodeList;

    /// Flag indicating whether or not a major change is happening to the bookmark tree.
    /// If true, the flattened bookmark list will not update itself until this flag is set back to false.
    bool m_canUpdateList;

    /// Next unique identifier to be assigned to a bookmark
    int m_nextBookmarkId;

    /// Stores the number of bookmarks in the tree.
    std::atomic_int m_numBookmarks;

    /// Future associated with the m_nodeList regeneration method
    QFuture<void> m_nodeListFuture;

    /// Mutex
    mutable std::mutex m_mutex;
};

#endif // BOOKMARKNODEMANAGER_H
