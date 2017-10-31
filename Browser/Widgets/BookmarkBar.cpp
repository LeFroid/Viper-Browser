#include "BookmarkBar.h"
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

    BookmarkFolder *folder = m_bookmarkManager->getRoot();
    for (BookmarkFolder *subFolder : folder->folders)
    {
        QToolButton *button = new QToolButton(this);
        button->setPopupMode(QToolButton::InstantPopup);
        button->setArrowType(Qt::DownArrow);
        button->setText(subFolder->name);

        QMenu *menu = new QMenu(this);
        addFolderItems(menu, subFolder, iconStorage);

        button->setMenu(menu);
        button->setToolButtonStyle(Qt::ToolButtonTextOnly);
        addWidget(button);
    }
    for (Bookmark *bookmark : folder->bookmarks)
    {
        QToolButton *button = new QToolButton(this);
        button->setText(bookmark->name);
        button->setIcon(iconStorage->getFavicon(bookmark->URL));
        button->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
        connect(button, &QToolButton::clicked, [=](){ emit loadBookmark(QUrl::fromUserInput(bookmark->URL)); });
        addWidget(button);
        //QAction *a = addAction(iconStorage->getFavicon(bookmark->URL), bookmark->name, [=](){ emit loadBookmark(QUrl::fromUserInput(bookmark->URL)); });
    }
}

void BookmarkBar::addFolderItems(QMenu *menu, BookmarkFolder *folder, FaviconStorage *iconStorage)
{
    if (!folder)
        return;

    for (BookmarkFolder *subFolder : folder->folders)
    {
        QMenu *subMenu = menu->addMenu(subFolder->name);
        addFolderItems(subMenu, subFolder, iconStorage);
    }

    for (Bookmark *bookmark : folder->bookmarks)
    {
        menu->addAction(iconStorage->getFavicon(bookmark->URL), bookmark->name, this, [=](){
            emit loadBookmark(QUrl::fromUserInput(bookmark->URL));
        });
    }
}
