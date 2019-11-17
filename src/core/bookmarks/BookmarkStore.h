#ifndef BOOKMARKSTORE_H
#define BOOKMARKSTORE_H

#include "DatabaseWorker.h"
#include "LRUCache.h"

#include <map>
#include <memory>
#include <string>
#include <vector>

#include <QObject>
#include <QString>

class BookmarkNode;
class BookmarkManager;

/**
 * @defgroup Bookmarks Bookmark System
 */

/**
 * @class BookmarkStore
 * @brief Persists the state of a user's bookmark collection throughout
 *        multiple browsing sessions.
 * @ingroup Bookmarks
 */
class BookmarkStore : public DatabaseWorker
{
    friend class BookmarkManager;
    friend class DatabaseFactory;

public:
    /// Bookmark constructor -5 loads database information into memory
    explicit BookmarkStore(const QString &databaseFile);

    /// Destructor
    ~BookmarkStore();

    /// Returns a pointer to the root node that contains a bookmark collection
    std::shared_ptr<BookmarkNode> getRootNode() const;

    /// Returns the largest unique id in the bookmark repository. Used by the bookmark manager to assign
    /// the next unique id when a bookmark is created
    int getMaxUniqueId() const;

    /// Inserts or replaces the given bookmark node into the database
    void insertNode(int nodeId, int parentId, int nodeType, const QString &name, const QUrl &url, int position);

    /// Removes a node from the database with the given id, parent id and position
    void removeNode(int nodeId, int parentId, int position);

    /// Saves a change in one or more properties of the given node
    void updateNode(int nodeId, int parentId, const QString &name, const QUrl &url, const QString &shortcut, int position);

private:
    /// Loads bookmark information from the database
    void loadFolder(BookmarkNode *folder);

    /// Saves all bookmarks to the database
    void save();

protected:
    /// Returns true if the bookmark database contains the table structure(s) needed for it to function properly,
    /// false if else.
    bool hasProperStructure() override;

    /// Creates the initial table structures and default bookmarks if necessary
    void setup() override;

    /// Loads bookmarks from the database
    void load() override;

private:
    /// Root bookmark folder
    std::shared_ptr<BookmarkNode> m_rootNode;
};

#endif // BOOKMARKSTORE_H

