#include "BookmarkImporter.h"
#include "BookmarkNode.h"

#include <stack>
#include <utility>
#include <QDebug>
#include <QFile>
#include <QUrl>

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

    if (contents.isEmpty())
        return false;

    QString pageHtml = QString::fromUtf8(contents);
    const int pageHtmlSize = pageHtml.size();

    // Find first <DL><p> for root folder
    int pos = pageHtml.indexOf(m_folderStartTag, 0, Qt::CaseInsensitive);
    if (pos == -1)
        return false;
    pos += m_folderStartTag.size();

    BookmarkNode *currentNode = importFolder;
    std::stack<BookmarkNode*> s;

    m_bookmarkManager->setCanUpdateList(false);
    while (pos > 0 && pos < pageHtmlSize && currentNode != nullptr)
    {
        pos = pageHtml.indexOf(m_startTag, pos);
        if (pos == -1)
            break;

        // Determine element type by comparing pos to position of the next sub-folder, bookmark and closing folder tag
        int nextFolderStartPos = pageHtml.indexOf(m_folderNameStartTag, pos, Qt::CaseInsensitive);
        if (pos == nextFolderStartPos)
        {
            // Skip folder attributes and only get the name, contained within the <h3>element</h3>
            pos = pageHtml.indexOf(m_endTag, pos + m_folderNameStartTag.size()) + 1;
            int nameEndPos = pageHtml.indexOf(m_folderNameEndTag, pos, Qt::CaseInsensitive);
            if (nameEndPos < 0)
            {
                qDebug() << "Error: invalid bookmark html. Halting import";
                m_bookmarkManager->setCanUpdateList(true);
                return false;
            }

            QString folderName = pageHtml.mid(pos, nameEndPos - pos);

            // Create bookmark folder for the child element and start parsing child node
            s.push(currentNode);
            currentNode = m_bookmarkManager->addFolder(folderName, currentNode);
            pos = pageHtml.indexOf(m_folderStartTag, nameEndPos + m_folderNameEndTag.size(), Qt::CaseInsensitive);
            if (pos > 0)
                pos += m_folderStartTag.size();
            continue;
        }

        int nextFolderEndPos = pageHtml.indexOf(m_folderEndTag, pos - 1, Qt::CaseInsensitive);
        if (pos == nextFolderEndPos)
        {
            // Current folder is done being imported, retrieve last folder from the stack and resume importing of that folder
            pos += m_folderEndTag.size();
            if (!s.empty())
            {
                currentNode = s.top();
                s.pop();
            }
            else
                currentNode = nullptr;
            continue;
        }

        int nextBookmarkPos = pageHtml.indexOf(m_bookmarkStartTag, pos, Qt::CaseInsensitive);
        if (pos == nextBookmarkPos)
        {
            // Get bookmark URL and name, add bookmark to current folder
            pos += m_bookmarkStartTag.size() - 1;
            int attrEndPos = pageHtml.indexOf(m_endTag, pos);
            if (attrEndPos < 0)
            {
                qDebug() << "Error: invalid bookmark html. Halting import";
                m_bookmarkManager->setCanUpdateList(true);
                return false;
            }

            // URL parsing
            QString attributeStr = pageHtml.mid(pos, attrEndPos - pos);
            int urlStartPos = attributeStr.indexOf(QStringLiteral("HREF="), 0, Qt::CaseInsensitive);
            if (urlStartPos < 0)
            {
                qDebug() << "Error: invalid bookmark html. Halting import";
                m_bookmarkManager->setCanUpdateList(true);
                return false;
            }
            urlStartPos += 6;
            int urlEndPos = attributeStr.indexOf(u'"', urlStartPos);
            if (urlEndPos < 0)
            {
                qDebug() << "Error: invalid bookmark html. Halting import";
                m_bookmarkManager->setCanUpdateList(true);
                return false;
            }

            QString url = attributeStr.mid(urlStartPos, urlEndPos - urlStartPos);

            // Bookmark name
            pos = attrEndPos + 1;
            int nameEndPos = pageHtml.indexOf(m_bookmarkEndTag, pos, Qt::CaseInsensitive);

            QString name = pageHtml.mid(pos, nameEndPos - pos);

            m_bookmarkManager->appendBookmark(name, QUrl::fromUserInput(url), currentNode);

            pos = nameEndPos + m_bookmarkEndTag.size();
            continue;
        }
        ++pos;
    }
    m_bookmarkManager->setCanUpdateList(true);

    return true;
}
