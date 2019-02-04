#ifndef BOOKMARKSTORE_H
#define BOOKMARKSTORE_H

#include "DatabaseWorker.h"
#include "LRUCache.h"
#include "ServiceLocator.h"

#include <map>
#include <memory>
#include <string>
#include <vector>

#include <QObject>
#include <QSqlQuery>
#include <QString>

class BookmarkNode;
class BookmarkManager;
class FaviconStore;

/**
 * @defgroup Bookmarks Bookmark System
 */

/**
 * @class BookmarkStore
 * @brief Persists the state of a user's bookmark collection throughout
 *        multiple browsing sessions.
 * @ingroup Bookmarks
 */
class BookmarkStore : public QObject, private DatabaseWorker
{
    Q_OBJECT

    friend class BookmarkImporter;
    friend class BookmarkTableModel;
    friend class BookmarkFolderModel;
    friend class DatabaseFactory;

public:
    /// Bookmark constructor - loads database information into memory
    explicit BookmarkStore(ViperServiceLocator &serviceLocator, const QString &databaseFile);

    /// Destructor
    ~BookmarkStore();

    /// Returns the in-memory manager and model of bookmarks
    BookmarkManager *getNodeManager() const;

signals:
    /// Emitted when there has been a change to the bookmark tree
    void bookmarksChanged();

private slots:
    /// Handles a change of one or more properties to the given bookmark node
    void onBookmarkChanged(const BookmarkNode *node);

    /// Inserts the given bookmark node into the database
    void onBookmarkCreated(const BookmarkNode *node);

    /// Removes the given bookmark from the database
    void onBookmarkDeleted(int uniqueId, int parentId, int position);

private:
    /// Loads bookmark information from the database
    void loadFolder(BookmarkNode *folder);

protected:
    /// Returns true if the bookmark database contains the table structure(s) needed for it to function properly,
    /// false if else.
    bool hasProperStructure() override;

    /// Creates the initial table structures and default bookmarks if necessary
    void setup() override;

    /// Saves all bookmarks to the database
    void save() override;

    /// Loads bookmarks from the database
    void load() override;

private:
    /// Root bookmark folder
    std::unique_ptr<BookmarkNode> m_rootNode;

    /// Handles in-app management of bookmarks
    BookmarkManager *m_nodeManager;

    /// Pointer to the favicon store
    FaviconStore *m_faviconStore;
};

#endif // BOOKMARKSTORE_H

