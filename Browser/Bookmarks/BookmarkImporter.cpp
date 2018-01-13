#include "BookmarkImporter.h"
#include "BookmarkNode.h"

#include <utility>
#include <QFile>
#include <QQueue>
#include <QWebElement>
#include <QWebFrame>
#include <QWebPage>

BookmarkImporter::BookmarkImporter(BookmarkManager *bookmarkMgr) :
    m_bookmarkManager(bookmarkMgr)
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

    // Load contents into a QWebPage to process bookmarks in its tree structure
    QWebPage page;
    QWebFrame *pageFrame = page.mainFrame();
    pageFrame->setHtml(QString::fromUtf8(contents));

    // Fetch root folder in bookmark page, add its first child to queue, and traverse the bookmark tree
    QWebElement folderElem = pageFrame->findFirstElement("DL");
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

    return true;
}
