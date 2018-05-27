#include "BookmarkImporter.h"
#include "BookmarkNode.h"

#include <stack>
#include <utility>
#include <QFile>
#include <QQueue>

BookmarkImporter::BookmarkImporter(BookmarkManager *bookmarkMgr) :
    m_bookmarkManager(bookmarkMgr),
    m_folderStartTag("<DL><p>"),
    m_folderEndTag("</DL><p>"),
    m_folderNameStartTag("<DT><H3"),
    m_folderNameEndTag("</H3>"),
    m_bookmarkStartTag("<DT><A"),
    m_bookmarkEndTag("</A>"),
    m_startTag('<'),
    m_endTag('>')
{
}

bool BookmarkImporter::import(const QString &fileName, BookmarkNode *importFolder)
{
    if (!importFolder)
        return false;

    // Attempt to read bookmark file contents
    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return false;

    QByteArray contents = file.readAll();
    file.close();
    if (contents.isEmpty() || contents.isNull())
        return false;

    QString pageHtml = QString::fromUtf8(contents);

    // Find first <DL><p> for root folder (which will be importFolder for bookmark import)
    int startPos = pageHtml.indexOf(m_folderStartTag, 0, Qt::CaseInsensitive);
    if (startPos == -1)
        return false;

    int endPos = pageHtml.lastIndexOf(m_folderEndTag, -1, Qt::CaseInsensitive);
    if (endPos == -1)
        return false;

    QString innerRootHtml = pageHtml.mid(startPos, endPos - startPos);
    int htmlSize = innerRootHtml.size();

    std::stack<BookmarkNode*> s;
    s.push(importFolder);

    //todo: keep track of the parser state, such as State::ReadingStartTag, State::ReadingEndTag, State::ReadingElementValue, etc.
    // set i += length of attributes while reading attributes of elements
    // the current folder with which bookmarks and subfolders will be appended is at the top of stack s.
    // when the closing tag of a folder is found, pop the top element from the stack
    // when the stack is empty, stop importing bookmarks
    /*
    for (int i = 0; i < htmlSize; ++i)
    {
        const QChar c = innerRootHtml.at(i);
    }
    */

    // Fetch root folder in bookmark page, add its first child to queue, and traverse the bookmark tree
   /* QWebElement folderElem = pageFrame->findFirstElement("DL");
    QWebElement subFolder = folderElem.firstChild();

    QQueue< std::pair<QWebElement, BookmarkNode*> > q;
    q.enqueue(std::make_pair(subFolder, importFolder));
    while (!q.empty())
    {
        // Currently in the first child node of a bookmark folder.
        auto p = q.dequeue();
        folderElem = p.first;
        BookmarkNode *folder = p.second;

        // Check each sibling if it matches the signature of a bookmark or a folder.
        // If the current node is a bookmark, add it to the folder. If it is a folder,
        // place the node at the end of the queue. Otherwise, ignore the element
        while (!folderElem.isNull())
        {
            if (folderElem.tagName().toLower().compare("dt") == 0)
            {
                // Check if next tag is 'h3' (representing a subfolder) or 'a' (representing a bookmark)
                folderElem = folderElem.firstChild();
                QString name = folderElem.tagName().toLower();
                if (name.compare("h3") == 0)
                {
                    // Create a subfolder
                    BookmarkNode *newFolder = m_bookmarkManager->addFolder(folderElem.toPlainText(), folder);
                    folderElem = folderElem.nextSibling();

                    subFolder = folderElem.firstChild();
                    q.enqueue(std::make_pair(subFolder, newFolder));
                }
                else if (name.compare("a") == 0)
                {
                    QString url = folderElem.attribute("HREF");
                    if (!url.isNull())
                    {
                        m_bookmarkManager->appendBookmark(folderElem.toPlainText(), url, folder);
                    }
                }
                folderElem = folderElem.parent().nextSibling();
            }
            else
                folderElem = folderElem.nextSibling();
        }
    }
*/

    return true;
}
