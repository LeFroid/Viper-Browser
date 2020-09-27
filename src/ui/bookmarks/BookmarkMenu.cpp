#include "BookmarkMenu.h"
#include "BookmarkManager.h"
#include "BookmarkNode.h"

#include <deque>

BookmarkMenu::BookmarkMenu(QWidget *parent) :
    QMenu(parent),
    m_addPageBookmarks(new QAction(tr("Bookmark this page"), parent)),
    m_removePageBookmarks(new QAction(tr("Remove this bookmark"), parent)),
    m_bookmarkManager(nullptr)
{
}

BookmarkMenu::BookmarkMenu(const QString &title, QWidget *parent) :
    QMenu(title, parent),
    m_addPageBookmarks(new QAction(tr("Bookmark this page"), parent)),
    m_removePageBookmarks(new QAction(tr("Remove this bookmark"), parent)),
    m_bookmarkManager(nullptr)
{
}

BookmarkMenu::~BookmarkMenu()
{
}

void BookmarkMenu::setBookmarkManager(BookmarkManager *bookmarkManager)
{
    m_bookmarkManager = bookmarkManager;
    setup();
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

    if (m_bookmarkManager->getRoot() == nullptr)
        return;

    // Iteratively load bookmark data into menu
    std::deque< std::pair<BookmarkNode*, QMenu*> > folders;
    folders.push_back({ m_bookmarkManager->getRoot(), this });

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
            connect(item, &QAction::triggered, [this, link](){
                emit loadUrlRequest(link);
            });
        }

        folders.pop_front();
    }
}

void BookmarkMenu::setup()
{
    connect(m_addPageBookmarks,    &QAction::triggered, parent(), [this](){ emit addPageToBookmarks(); });
    connect(m_removePageBookmarks, &QAction::triggered, parent(), [this](){ emit removePageFromBookmarks(false); });

    connect(m_bookmarkManager, &BookmarkManager::bookmarksChanged, this, &BookmarkMenu::resetMenu);

    resetMenu();
}
