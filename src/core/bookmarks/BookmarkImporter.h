#ifndef BOOKMARKIMPORTER_H
#define BOOKMARKIMPORTER_H

#include "BookmarkManager.h"

#include <QLatin1String>

/**
 * @class BookmarkImporter
 * @brief Parses Netscape HTML formatted bookmarks, importing them
 *        into the user's bookmark system
 * @ingroup Bookmarks
 */
class BookmarkImporter
{
public:
    /// Constructs the bookmark importer, given a pointer to the bookmark node manager
    explicit BookmarkImporter(BookmarkManager *bookmarkMgr);

    /**
     * @brief import Attempts to import bookmarks from the given HTML file into a bookmark folder
     * @param fileName File containing Netscape formatted bookmark data
     * @param importFolder Root folder to import bookmarks into
     * @return True on successful import, false on failure
     */
    bool import(const QString &fileName, BookmarkNode *importFolder);

private:
    /// Bookmark node manager
    BookmarkManager *m_bookmarkManager;

    /// Folder element opening and closing tags
    QLatin1String m_folderStartTag;
    QLatin1String m_folderEndTag;

    /// Folder name element opening and closing tags
    QLatin1String m_folderNameStartTag;
    QLatin1String m_folderNameEndTag;

    /// Bookmark element opening and closing tags
    QLatin1String m_bookmarkStartTag;
    QLatin1String m_bookmarkEndTag;

    /// XML element opening and closing characters
    QChar m_startTag;
    QChar m_endTag;
};

#endif // BOOKMARKIMPORTER_H
