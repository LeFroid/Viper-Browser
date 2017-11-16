#include "BookmarkBar.h"
#include "BookmarkNode.h"
#include "BrowserApplication.h"
#include "FaviconStorage.h"
#include <QMenu>
#include <QPushButton>
#include <QToolButton>
#include <QUrl>

BookmarkBar::BookmarkBar(QWidget *parent) :
    QToolBar(tr("Bookmark Bar"), parent),
    m_bookmarkManager(nullptr)
{
}

void BookmarkBar::setBookmarkManager(std::shared_ptr<BookmarkManager> manager)
{
    m_bookmarkManager = manager;
    refresh();
}

void BookmarkBar::refresh()
{
    if (!m_bookmarkManager.get())
        return;

    clear();

    FaviconStorage *iconStorage = sBrowserApplication->getFaviconStorage();

    BookmarkNode *folder = m_bookmarkManager->getRoot();
    int numChildren = folder->getNumChildren();
    for (int i = 0; i < numChildren; ++i)
    {
        BookmarkNode *child = folder->getNode(i);
        if (child->getType() == BookmarkNode::Folder)
        {
            QToolButton *button = new QToolButton(this);
            button->setPopupMode(QToolButton::InstantPopup);
            button->setArrowType(Qt::DownArrow);
            button->setText(child->getName());
            button->setIcon(child->getIcon());

            QMenu *menu = new QMenu(this);
            addFolderItems(menu, child, iconStorage);

            button->setMenu(menu);
            button->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
            addWidget(button);
        }
        else
        {
            QToolButton *button = new QToolButton(this);
            button->setText(child->getName());
            button->setIcon(iconStorage->getFavicon(QUrl(child->getURL())));
            button->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
            connect(button, &QToolButton::clicked, [=](){ emit loadBookmark(QUrl::fromUserInput(child->getURL())); });
            addWidget(button);
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
