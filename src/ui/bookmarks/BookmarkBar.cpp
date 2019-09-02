#include "BookmarkBar.h"
#include "BookmarkNode.h"
#include "BookmarkManager.h"
#include "BrowserApplication.h"
#include "MainWindow.h"

#include <QFont>
#include <QHBoxLayout>
#include <QMenu>
#include <QPushButton>
#include <QResizeEvent>
#include <QSpacerItem>
#include <QStyle>
#include <QUrl>

#include <QtGlobal>

BookmarkBar::BookmarkBar(QWidget *parent) :
    QWidget(parent),
    m_bookmarkManager(nullptr),
    m_layout(new QHBoxLayout(this))
{
    setMinimumHeight(32);
    setMaximumHeight(32);
    m_layout->setContentsMargins(2, 0, 0, 2);
    m_layout->setSpacing(2);
    setLayout(m_layout);

    installEventFilter(this);
    m_layout->installEventFilter(this);
}

void BookmarkBar::setBookmarkManager(BookmarkManager *manager)
{
    m_bookmarkManager = manager;
    connect(m_bookmarkManager, &BookmarkManager::bookmarksChanged, this, &BookmarkBar::refresh);
    refresh();
}

bool BookmarkBar::eventFilter(QObject *watched, QEvent *event)
{
    if (!isVisible() || (watched != this && watched != m_layout))
        return QObject::eventFilter(watched, event);

    switch(event->type())
    {
        case QEvent::Resize:
        {
            // If available size is shrinking significantly, recreate the bookmark bar items
            QResizeEvent *ev = static_cast<QResizeEvent*>(event);
            int deltaW = std::abs(ev->oldSize().width() - ev->size().width());
            if (deltaW >= 100)
                refresh();
            break;
        }
        default:
            break;
    }

    return QObject::eventFilter(watched, event);
}

QSize BookmarkBar::minimumSizeHint() const
{
    return QSize(128, 32);
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

    BookmarkNode *folder = m_bookmarkManager->getBookmarksBar();
    if (!folder)
        return;

    int numChildren = folder->getNumChildren();

    QFontMetrics fMetrics = fontMetrics();
#if (QT_VERSION >= QT_VERSION_CHECK(5, 11, 0))
    const int maxTextWidth = fMetrics.horizontalAdvance(QLatin1Char('R')) * 12;
#else
    const int maxTextWidth = fMetrics.width(QLatin1Char('R')) * 12;
#endif

    int spaceLeft = window()->width();

    // Show contents of bookmarks bar
    for (int i = 0; i < numChildren; ++i)
    {
        BookmarkNode *child = folder->getNode(i);
        const QString childText = fMetrics.elidedText(child->getName(), Qt::ElideRight, maxTextWidth);

        // Set common properties of button before folder or bookmark specific properties
        QPushButton *button = new QPushButton(this);
        button->setFlat(true);
        button->setIcon(child->getIcon());
        button->setText(childText);
        button->setProperty("BookmarkNode", QVariant::fromValue<BookmarkNode*>(child));
        button->setSizePolicy(QSizePolicy::Minimum, button->sizePolicy().verticalPolicy());
        button->setContextMenuPolicy(Qt::CustomContextMenu);
        connect(button, &QPushButton::customContextMenuRequested, this, &BookmarkBar::showBookmarkContextMenu);

        if (child->getType() == BookmarkNode::Folder)
        {
            QMenu *menu = new QMenu(this);
            addFolderItems(menu, child);
            button->setMenu(menu);
        }
        else
        {
            connect(button, &QPushButton::clicked, [=](){ emit loadBookmark(child->getURL()); });
        }

        m_layout->addWidget(button, 0, Qt::AlignLeft);

        // Check if there is enough space for the rest of the bookmarks in the bar
        spaceLeft -= (button->width() + 4);
        if (i > 12 || spaceLeft <= button->width() * 3 / 2)
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
                    addFolderItems(subMenu, child);
                }
                else
                {
                    QUrl nodeUrl = child->getURL();
                    extraBookmarksMenu->addAction(child->getIcon(), child->getName(), this, [this, nodeUrl](){
                        emit loadBookmark(nodeUrl);
                    });
                }
            }

            m_layout->addSpacing(button->width() / 6);
            m_layout->addWidget(extraBookmarksButton, 0, Qt::AlignRight);
            break;
        }
    }
}

void BookmarkBar::addFolderItems(QMenu *menu, BookmarkNode *folder)
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
            addFolderItems(subMenu, child);
        }
        else
        {
            QUrl nodeUrl = child->getURL();
            menu->addAction(child->getIcon(), child->getName(), this, [this, nodeUrl](){
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
    int numChildren = folder->getNumChildren();
    for (int i = 0; i < numChildren; ++i)
    {
        child = folder->getNode(i);
        if (child->getType() != BookmarkNode::Bookmark)
            continue;

        emit loadBookmarkNewTab(child->getURL());
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
    int numChildren = folder->getNumChildren();
    for (int i = 0; i < numChildren; ++i)
    {
        child = folder->getNode(i);
        if (child->getType() != BookmarkNode::Bookmark)
            continue;

        if (firstTab)
        {
            win->loadUrl(child->getURL());
            firstTab = false;
        }
        else
            win->openLinkNewTab(child->getURL());
    }
}
