#include "BookmarkImporter.h"

#include <QFile>
#include <QQueue>
#include <QWebElement>
#include <QWebFrame>
#include <QWebPage>

#include <QDebug>

BookmarkImporter::BookmarkImporter(std::shared_ptr<BookmarkManager> bookmarkMgr) :
    m_bookmarkManager(bookmarkMgr)
{
}

bool BookmarkImporter::import(const QString &fileName, BookmarkFolder *importFolder)
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
    subFolder.setAttribute("FolderID", QString::number(importFolder->id));
    int folderId = -1;

    QQueue<QWebElement> q;
    q.enqueue(subFolder);
    while (!q.empty())
    {
        // Currently in the first child node of a bookmark folder.
        folderElem = q.dequeue();
        folderId = folderElem.attribute("FolderID").toInt();

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
                    int subFolderId = m_bookmarkManager->addFolder(folderElem.toPlainText(), folderId);
                    folderElem = folderElem.nextSibling();

                    subFolder = folderElem.firstChild();
                    subFolder.setAttribute("FolderID", QString::number(subFolderId));
                    q.enqueue(subFolder);
                }
                else if (name.compare("a") == 0)
                {
                    QString url = folderElem.attribute("HREF");
                    if (!url.isNull())
                    {
                        m_bookmarkManager->addBookmark(folderElem.toPlainText(), url, folderId);
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
