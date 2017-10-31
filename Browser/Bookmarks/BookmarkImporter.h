#ifndef BOOKMARKIMPORTER_H
#define BOOKMARKIMPORTER_H

#include "BookmarkManager.h"
#include <memory>

/**
 * @class BookmarkImporter
 * @brief Parses Netscape HTML formatted bookmarks, importing them
 *        into the user's bookmark system
 */
class BookmarkImporter
{
public:
    /// Constructs the bookmark importer, given a shared pointer to the bookmark manager
    explicit BookmarkImporter(std::shared_ptr<BookmarkManager> bookmarkMgr);

    /**
     * @brief import Attempts to import bookmarks from the given HTML file into a bookmark folder
     * @param fileName File containing Netscape formatted bookmark data
     * @param importFolder Root folder to import bookmarks into
     * @return True on successful import, false on failure
     */
    bool import(const QString &fileName, BookmarkFolder *importFolder);

private:
    /// Bookmark manager
    std::shared_ptr<BookmarkManager> m_bookmarkManager;
};

#endif // BOOKMARKIMPORTER_H
