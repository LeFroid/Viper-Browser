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
class BookmarkNodeManager;

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
    /// Bookmark constructor - loads database information into memory
    explicit BookmarkManager(const QString &databaseFile, QObject *parent = nullptr);

    /// Destructor
    ~BookmarkManager();

    /// Returns the in-memory manager and model of bookmarks
    BookmarkNodeManager *getNodeManager() const;

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
    BookmarkNodeManager *m_nodeManager;
};

#endif // BOOKMARKMANAGER_H
