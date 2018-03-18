#include "BookmarkBar.h"
#include "BookmarkNode.h"
#include "BrowserApplication.h"
#include "FaviconStorage.h"
#include "MainWindow.h"

#include <QFont>
#include <QMenu>
#include <QPushButton>
#include <QToolButton>
#include <QUrl>

BookmarkBar::BookmarkBar(QWidget *parent) :
    QToolBar(tr("Bookmark Bar"), parent),
    m_bookmarkManager(nullptr)
{
}

void BookmarkBar::setBookmarkManager(BookmarkManager *manager)
{
    m_bookmarkManager = manager;
    refresh();
}

void BookmarkBar::showBookmarkContextMenu(const QPoint &pos)
{
    QToolButton *button = qobject_cast<QToolButton*>(sender());
    BookmarkNode *node = button->property("BookmarkNode").value<BookmarkNode*>();
    if (!button || !node)
        return;

    QMenu menu(this);

    switch (node->getType())
    {
        case BookmarkNode::Bookmark:
        {
            QUrl bookmarkUrl(node->getURL());

            QAction *openAction = menu.addAction(tr("Open"), [=](){ emit loadBookmark(bookmarkUrl); });
            QFont openFont = openAction->font();
            openFont.setBold(true);
            openAction->setFont(openFont);
            menu.addAction(tr("Open in a new tab"), [=](){ emit loadBookmarkNewTab(bookmarkUrl); });
            menu.addAction(tr("Open in a new window"), [=](){ emit loadBookmarkNewWindow(bookmarkUrl); });
            break;
        }
        case BookmarkNode::Folder:
        {
            menu.addAction(tr("Open all items in tabs"), [=](){ openFolderItemsNewTabs(node); });
            menu.addAction(tr("Open all items in new window"), [=](){ openFolderItemsNewWindow(node); });
            menu.addAction(tr("Open all items in private window"), [=](){ openFolderItemsNewWindow(node, true); });
            break;
        }
    }

    menu.exec(mapToGlobal(pos));
}

void BookmarkBar::refresh()
{
    if (!m_bookmarkManager)
        return;

    clear();

    FaviconStorage *iconStorage = sBrowserApplication->getFaviconStorage();

    BookmarkNode *folder = m_bookmarkManager->getBookmarksBar();
    int numChildren = folder->getNumChildren();

    QFontMetrics fMetrics = fontMetrics();
    const int maxTextWidth = fMetrics.width(QLatin1Char('R')) * 16;

    // Show contents of bookmarks bar
    for (int i = 0; i < numChildren; ++i)
    {
        BookmarkNode *child = folder->getNode(i);
        const QString childText = fMetrics.elidedText(child->getName(), Qt::ElideRight, maxTextWidth);

        // Set common properties of button before folder or bookmark specific properties
        QToolButton *button = new QToolButton(this);
        button->setText(childText);
        button->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
        button->setProperty("BookmarkNode", QVariant::fromValue<BookmarkNode*>(child));
        button->setSizePolicy(QSizePolicy::Minimum, button->sizePolicy().verticalPolicy());
        button->setContextMenuPolicy(Qt::CustomContextMenu);
        connect(button, &QToolButton::customContextMenuRequested, this, &BookmarkBar::showBookmarkContextMenu);

        if (child->getType() == BookmarkNode::Folder)
        {
            button->setPopupMode(QToolButton::InstantPopup);
            //button->setArrowType(Qt::DownArrow);
            button->setIcon(child->getIcon());

            QMenu *menu = new QMenu(this);
            addFolderItems(menu, child, iconStorage);
            button->setMenu(menu);
        }
        else
        {
            button->setIcon(iconStorage->getFavicon(QUrl(child->getURL())));
            connect(button, &QToolButton::clicked, [=](){ emit loadBookmark(QUrl::fromUserInput(child->getURL())); });
        }

        addWidget(button);
    }
}

void BookmarkBar::addFolderItems(QMenu *menu, BookmarkNode *folder, FaviconStorage *iconStorage)
{
    if (!folder)
        return;

    int numChildren = folder->getNumChildren();
    for (int i = 0; i < numChildren; ++i)
    {
        BookmarkNode *child = folder->getNode(i);
        if (child->getType() == BookmarkNode::Folder)
        {
            QMenu *subMenu = menu->addMenu(child->getIcon(), child->getName());
            addFolderItems(subMenu, child, iconStorage);
        }
        else
        {
            QUrl nodeUrl(child->getURL());
            menu->addAction(iconStorage->getFavicon(nodeUrl), child->getName(), this, [=](){
                emit loadBookmark(nodeUrl);
            });
        }
    }
}

void BookmarkBar::openFolderItemsNewTabs(BookmarkNode *folder)
{
    if (!folder)
        return;

    BookmarkNode *child = nullptr;
    QUrl url;
    int numChildren = folder->getNumChildren();
    for (int i = 0; i < numChildren; ++i)
    {
        child = folder->getNode(i);
        if (child->getType() != BookmarkNode::Bookmark)
            continue;

        url = QUrl(child->getURL());
        emit loadBookmarkNewTab(url);
    }
}

void BookmarkBar::openFolderItemsNewWindow(BookmarkNode *folder, bool privateWindow)
{
    if (!folder)
        return;

    MainWindow *win = nullptr;
    if (privateWindow)
        win = sBrowserApplication->getNewPrivateWindow();
    else
        win = sBrowserApplication->getNewWindow();

    bool firstTab = true;
    BookmarkNode *child = nullptr;
    QUrl url;
    int numChildren = folder->getNumChildren();
    for (int i = 0; i < numChildren; ++i)
    {
        child = folder->getNode(i);
        if (child->getType() != BookmarkNode::Bookmark)
            continue;

        url = QUrl(child->getURL());
        if (firstTab)
        {
            win->loadUrl(url);
            firstTab = false;
        }
        else
            win->openLinkNewTab(url);
    }
}
