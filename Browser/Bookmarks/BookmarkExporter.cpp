#include "BookmarkExporter.h"

const QString BookmarkExporter::NetscapeHeader = QString("<!DOCTYPE NETSCAPE-Bookmark-file-1>\n"
                                       "<!-- This is an automatically generated file.\n"
                                       "     It will be read and overwritten.\n"
                                       "     DO NOT EDIT! -->\n"
                                       "<META HTTP-EQUIV=\"Content-Type\" CONTENT=\"text/html; charset=UTF-8\">\n"
                                       "<TITLE>Bookmarks</TITLE>\n<H1>Bookmarks</H1>\n");

BookmarkExporter::BookmarkExporter(std::shared_ptr<BookmarkManager> bookmarkMgr) :
    m_bookmarkManager(bookmarkMgr),
    m_outputFile(),
    m_recursionLevel(0)
{
}

bool BookmarkExporter::saveTo(const QString &fileName)
{
    m_outputFile.setFileName(fileName);
    if (!m_outputFile.open(QIODevice::WriteOnly | QIODevice::Text))
        return false;

    m_recursionLevel = 0;

    QTextStream stream(&m_outputFile);
    stream << NetscapeHeader;

    // Recursively export bookmarks, starting with the root folder
    exportFolder(m_bookmarkManager->getRoot(), stream);

    stream.flush();
    m_outputFile.close();

    return true;
}

void BookmarkExporter::exportFolder(BookmarkFolder* folder, QTextStream &stream)
{
    if (!folder)
        return;

    QString spacing;
    for (int i = 0; i < m_recursionLevel; ++i)
        spacing.append("    ");

    stream << spacing << "<DL><p>\n";

    // Export sub-folders
    ++m_recursionLevel;
    for (BookmarkFolder *f : folder->folders)
    {
        stream << spacing << "    <DT><H3>" << f->name << "</H3>\n";
        exportFolder(f, stream);
    }

    // Export bookmarks belonging to current folder
    for (Bookmark *b : folder->bookmarks)
        stream << spacing << "    <DT><A HREF=\"" << b->URL << "\">" << b->name << "</A>\n";

    stream << spacing << "</DL><p>\n";
}
