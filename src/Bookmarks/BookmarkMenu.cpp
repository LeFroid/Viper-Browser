#include "BookmarkMenu.h"
#include "BrowserApplication.h"
#include "BookmarkManager.h"

#include <deque>

BookmarkMenu::BookmarkMenu(QWidget *parent) :
    QMenu(parent),
    m_addPageBookmarks(new QAction(tr("Bookmark this page"), parent)),
    m_removePageBookmarks(new QAction(tr("Remove this bookmark"), parent))
{
    setup();
}

BookmarkMenu::BookmarkMenu(const QString &title, QWidget *parent) :
    QMenu(title, parent),
    m_addPageBookmarks(new QAction(tr("Bookmark this page"), parent)),
    m_removePageBookmarks(new QAction(tr("Remove this bookmark"), parent))
{
    setup();
}

BookmarkMenu::~BookmarkMenu()
{
}

void BookmarkMenu::setCurrentPageBookmarked(bool state)
{
    removeAction(m_addPageBookmarks);
    removeAction(m_removePageBookmarks);

    QList<QAction*> menuActions = actions();
    if (menuActions.empty())
        return;
    int lastActionPos = (menuActions.size() > 1) ? 1 : 0;
    QAction *firstBookmark = menuActions[lastActionPos];
    QAction *actionToAdd = state ? m_removePageBookmarks : m_addPageBookmarks;
    insertAction(firstBookmark, actionToAdd);
}

void BookmarkMenu::resetMenu()
{
    clear();
    addAction(tr("Manage Bookmarks"), this, &BookmarkMenu::manageBookmarkRequest);
    addSeparator();

    // Iteratively load bookmark data into menu
    std::deque< std::pair<BookmarkNode*, QMenu*> > folders;
    folders.push_back({ sBrowserApplication->getBookmarkManager()->getRoot(), this });

    while (!folders.empty())
    {
        auto &current = folders.front();
        BookmarkNode *currentNode = current.first;
        QMenu *currentMenu = current.second;

        int numChildren = currentNode->getNumChildren();
        for (int i = 0; i < numChildren; ++i)
        {
            BookmarkNode *n = currentNode->getNode(i);
            if (n->getType() == BookmarkNode::Folder)
            {
                QMenu *subMenu = currentMenu->addMenu(n->getIcon(), n->getName());
                folders.push_back({n, subMenu});
                continue;
            }

            QUrl link(n->getURL());
            QAction *item = currentMenu->addAction(n->getIcon(), n->getName());
            item->setIconVisibleInMenu(true);
            currentMenu->addAction(item);
            connect(item, &QAction::triggered, [=](){ emit loadUrlRequest(link); });
        }

        folders.pop_front();
    }
}

void BookmarkMenu::setup()
{
    connect(m_addPageBookmarks,    &QAction::triggered, [=](){ emit addPageToBookmarks(); });
    connect(m_removePageBookmarks, &QAction::triggered, [=](){ emit removePageFromBookmarks(false); });

    resetMenu();
}
