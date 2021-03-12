#include "BookmarkExporter.h"
#include "BookmarkNode.h"

#include <deque>
#include <stack>

#include <QUrl>

const QString BookmarkExporter::NetscapeHeader = QStringLiteral("<!DOCTYPE NETSCAPE-Bookmark-file-1>\n"
                                       "<!-- This is an automatically generated file.\n"
                                       "     It will be read and overwritten.\n"
                                       "     DO NOT EDIT! -->\n"
                                       "<META HTTP-EQUIV=\"Content-Type\" CONTENT=\"text/html; charset=UTF-8\">\n"
                                       "<TITLE>Bookmarks</TITLE>\n<H1>Bookmarks</H1>\n");

BookmarkExporter::BookmarkExporter(BookmarkManager *bookmarkMgr) :
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

    // Iteratively export bookmarks
    exportFolders(stream);

    stream.flush();
    m_outputFile.close();

    return true;
}

void BookmarkExporter::exportFolders(QTextStream &stream)
{
    // Iteratively export bookmarks
    QString spacing;

    std::stack<std::pair<BookmarkNode*, int>> nodes;
    nodes.push({ m_bookmarkManager->getRoot(), 0 });
    while (!nodes.empty())
    {
        std::pair<BookmarkNode*, int> currentItem = nodes.top();
        BookmarkNode *current = currentItem.first;
        int depth = currentItem.second;
        nodes.pop();

        spacing.clear();
        for (int i = 0; i < depth; ++i)
            spacing.append(QStringLiteral("    "));

        if (depth > 0)
            stream << spacing << "<DT><H3>" << current->getName() << "</H3>\n";

        stream << spacing << "<DL><p>\n";

        int numChildren = current->getNumChildren();
        std::deque<BookmarkNode*> subFolders;
        for (int i = 0; i < numChildren; ++i)
        {
            BookmarkNode *n = current->getNode(i);
            if (n->getType() == BookmarkNode::Folder)
                subFolders.push_back(n);
            else
                stream << spacing << "    <DT><A HREF=\"" << n->getURL().toString(QUrl::FullyEncoded) << "\">" << n->getName() << "</A>\n";
        }

        // At max depth, form closing tags
        if (subFolders.empty())
        {
            // Number of closing tags = current depth - depth of next item on stack (or 0 if stack is empty)
            int closingTags = depth;
            if (!nodes.empty())
                closingTags += 1 - nodes.top().second;

            for (int i = closingTags; i > 0; --i)
            {
                stream << spacing << "</DL><p>\n";
                spacing = spacing.left(spacing.size() - 4);
            }
        }
        else
        {
            // Add subfolders to stack at this point in order to preserve the internal ordering for the user
            while (!subFolders.empty())
            {
                nodes.push({ subFolders.back(), depth + 1 });
                subFolders.pop_back();
            }
        }
    }

    // Root closing tag
    stream << spacing << "</DL><p>\n";
}
