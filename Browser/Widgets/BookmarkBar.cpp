#include "BookmarkBar.h"
#include "BookmarkNode.h"
#include "BrowserApplication.h"
#include "FaviconStorage.h"
#include "MainWindow.h"

#include <QFont>
#include <QHBoxLayout>
#include <QMenu>
#include <QPushButton>
#include <QSpacerItem>
#include <QStyle>
#include <QUrl>

BookmarkBar::BookmarkBar(QWidget *parent) :
    QWidget(parent),
    m_bookmarkManager(nullptr),
    m_layout(new QHBoxLayout(this))
{
    setMinimumHeight(32);
    setMaximumHeight(32);
    m_layout->setContentsMargins(2, 0, 0, 2);
    setLayout(m_layout);
}

void BookmarkBar::setBookmarkManager(BookmarkManager *manager)
{
    m_bookmarkManager = manager;
    refresh();
}

void BookmarkBar::showBookmarkContextMenu(const QPoint &pos)
{
    QPushButton *button = qobject_cast<QPushButton*>(sender());
    BookmarkNode *node = button->property("BookmarkNode").value<BookmarkNode*>();
    if (!button || !node)
        return;

    QMenu menu;

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

    menu.exec(button->mapToGlobal(pos));
}

void BookmarkBar::clear()
{
    while (QLayoutItem *child = m_layout->takeAt(0))
    {
        delete child->widget();
        delete child;
    }
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

    int spaceLeft = window()->width();

    // Show contents of bookmarks bar
    for (int i = 0; i < numChildren; ++i)
    {
        BookmarkNode *child = folder->getNode(i);
        const QString childText = fMetrics.elidedText(child->getName(), Qt::ElideRight, maxTextWidth);

        // Set common properties of button before folder or bookmark specific properties
        QPushButton *button = new QPushButton(this);
        button->setFlat(true);
        button->setText(childText);
        button->setProperty("BookmarkNode", QVariant::fromValue<BookmarkNode*>(child));
        button->setSizePolicy(QSizePolicy::Minimum, button->sizePolicy().verticalPolicy());
        button->setContextMenuPolicy(Qt::CustomContextMenu);
        connect(button, &QPushButton::customContextMenuRequested, this, &BookmarkBar::showBookmarkContextMenu);

        if (child->getType() == BookmarkNode::Folder)
        {
            button->setIcon(child->getIcon());

            QMenu *menu = new QMenu(this);
            addFolderItems(menu, child, iconStorage);
            button->setMenu(menu);
        }
        else
        {
            button->setIcon(iconStorage->getFavicon(QUrl(child->getURL())));
            connect(button, &QPushButton::clicked, [=](){ emit loadBookmark(QUrl::fromUserInput(child->getURL())); });
        }

        m_layout->addWidget(button, 0, Qt::AlignLeft);

        // Check if there is enough space for the rest of the bookmarks in the bar
        spaceLeft -= button->width();
        if (spaceLeft <= button->width())
        {
            QPushButton *extraBookmarksButton = new QPushButton(this);
            extraBookmarksButton->setFlat(true);
            extraBookmarksButton->setSizePolicy(QSizePolicy::Minimum, button->sizePolicy().verticalPolicy());
            extraBookmarksButton->setIcon(style()->standardIcon(QStyle::SP_ArrowForward, 0, this));

            QMenu *extraBookmarksMenu = new QMenu(this);
            extraBookmarksButton->setMenu(extraBookmarksMenu);

            for (int j = i + 1; j < numChildren; ++j)
            {
                child = folder->getNode(j);
                if (child->getType() == BookmarkNode::Folder)
                {
                    QMenu *subMenu = extraBookmarksMenu->addMenu(child->getIcon(), child->getName());
                    addFolderItems(subMenu, child, iconStorage);
                }
                else
                {
                    QUrl nodeUrl(child->getURL());
                    extraBookmarksMenu->addAction(iconStorage->getFavicon(nodeUrl), child->getName(), this, [=](){
                        emit loadBookmark(nodeUrl);
                    });
                }
            }

            m_layout->addWidget(extraBookmarksButton, 0, Qt::AlignRight);
            break;
        }
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
