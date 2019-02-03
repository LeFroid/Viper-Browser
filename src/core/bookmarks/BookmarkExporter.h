#ifndef BOOKMARKEXPORTER_H
#define BOOKMARKEXPORTER_H

#include "BookmarkManager.h"
#include <QFile>
#include <QString>
#include <QTextStream>

/**
 * @class BookmarkExporter
 * @brief Converts the user's bookmark data into an exportable HTML Netscape bookmark file format
 * @ingroup Bookmarks
 */
class BookmarkExporter
{
public:
    /// Constructs the bookmark exporter given a pointer to the BookmarkManager
    explicit BookmarkExporter(BookmarkManager *bookmarkMgr);

    /**
     * @brief saveTo Attempts to save the user's bookmarks to a file
     * @param fileName Name of the file in which the bookmarks will be exported
     * @return True on successful export, false on failure
     */
    bool saveTo(const QString &fileName);

private:
    /**
     * @brief exportFolder Iteratively exports bookmark data into the output file
     * @param stream Bookmark file text stream
     */
    void exportFolders(QTextStream &stream);

private:
    /// Netscape bookmark file header
    static const QString NetscapeHeader;

    /// Bookmark manager
    BookmarkManager *m_bookmarkManager;

    /// Output file handle
    QFile m_outputFile;

    /// Counts number of recursive calls to exportFolder(..) method to make the proper number of spaces in the html file
    int m_recursionLevel;
};

#endif // BOOKMARKEXPORTER_H
