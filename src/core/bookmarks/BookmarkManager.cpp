#include "BookmarkManager.h"
#include "BookmarkNode.h"
#include "BookmarkStore.h"
#include "FaviconStore.h"

#include <deque>
#include <memory>

#include <QtConcurrent>

BookmarkManager::BookmarkManager(ViperServiceLocator &serviceLocator, QObject *parent) :
    QObject(parent),
    m_rootNode(nullptr),
    m_bookmarkBar(nullptr),
    m_faviconStore(nullptr),
    m_lookupCache(24),
    m_nodeList(),
    m_canUpdateList(true),
    m_numBookmarks(0),
    m_nodeListFuture(),
    m_mutex()
{
    m_faviconStore = serviceLocator.getServiceAs<FaviconStore>("FaviconStore");
    setObjectName(QLatin1String("BookmarkManager"));
}

BookmarkManager::~BookmarkManager()
{
    std::lock_guard<std::mutex> _(m_mutex);
    waitToFinishList();
}

BookmarkNode *BookmarkManager::getRoot() const
{
    return m_rootNode;
}

BookmarkNode *BookmarkManager::getBookmarksBar() const
{
    return m_bookmarkBar;
}

BookmarkNode *BookmarkManager::getBookmark(const QUrl &url)
{
    if (url.isEmpty())
        return nullptr;

    const std::string urlStdStr = url.toString().toStdString();
    if (m_lookupCache.has(urlStdStr))
    {
        BookmarkNode *node = m_lookupCache.get(urlStdStr);
        if (node != nullptr)
            return node;
    }

    if (!m_nodeListFuture.isCanceled() && m_nodeListFuture.isRunning())
        m_nodeListFuture.waitForFinished();

    for (BookmarkNode *node : m_nodeList)
    {
        if (node->getType() == BookmarkNode::Bookmark
                && node->getURL() == url)
            return node;
    }

    return nullptr;
}

bool BookmarkManager::isBookmarked(const QUrl &url)
{
    if (url.isEmpty())
        return false;

    const std::string urlStdStr = url.toString().toStdString();
    if (m_lookupCache.has(urlStdStr) && m_lookupCache.get(urlStdStr) != nullptr)
        return true;

    if (!m_nodeListFuture.isCanceled() && m_nodeListFuture.isRunning())
        m_nodeListFuture.waitForFinished();

    for (BookmarkNode *node : m_nodeList)
    {
        if (node->m_url == url)
        {
            m_lookupCache.put(urlStdStr, node);
            return true;
        }
    }

    return false;
}

void BookmarkManager::appendBookmark(const QString &name, const QUrl &url, BookmarkNode *folder)
{
    // If parent folder not specified, set to root folder
    if (!folder)
        folder = getBookmarksBar();

    const int bookmarkId = m_numBookmarks.load();

    // Create new bookmark
    BookmarkNode *bookmark = folder->appendNode(std::make_unique<BookmarkNode>(BookmarkNode::Bookmark, name));
    bookmark->setUniqueId(bookmarkId);
    bookmark->setURL(url);
    bookmark->setIcon(m_faviconStore ? m_faviconStore->getFavicon(url) : QIcon());

    ++m_numBookmarks;

    emit bookmarkCreated(bookmark); //set the uniqueId of bookmark in the handler of this signal

    scheduleResetList();
}

void BookmarkManager::insertBookmark(const QString &name, const QUrl &url, BookmarkNode *folder, int position)
{
    if (!folder)
        folder = getBookmarksBar();

    if (position < 0 || position >= folder->getNumChildren())
    {
        appendBookmark(name, url, folder);
        return;
    }

    const int bookmarkId = m_numBookmarks.load();

    // Create new bookmark
    BookmarkNode *bookmark = folder->insertNode(std::make_unique<BookmarkNode>(BookmarkNode::Bookmark, name), position);
    bookmark->setUniqueId(bookmarkId);
    bookmark->setURL(url);
    bookmark->setIcon(m_faviconStore ? m_faviconStore->getFavicon(url) : QIcon());

    ++m_numBookmarks;

    emit bookmarkCreated(bookmark);

    scheduleResetList();
}

BookmarkNode *BookmarkManager::addFolder(const QString &name, BookmarkNode *parent)
{
    // If parent cannot be found then attach the new folder directly to the root folder
    if (!parent)
        parent = m_rootNode;

    const int folderId = m_numBookmarks.load();

    // Append bookmark folder to parent
    BookmarkNode *folder = parent->appendNode(std::make_unique<BookmarkNode>(BookmarkNode::Folder, name));
    folder->setUniqueId(folderId);
    folder->setIcon(QIcon::fromTheme(QLatin1String("folder")));
    ++m_numBookmarks;

    emit bookmarkCreated(folder);

    scheduleResetList();
    return folder;
}

void BookmarkManager::removeBookmark(const QUrl &url)
{
    if (url.isEmpty())
        return;

    if (m_lookupCache.has(url.toString().toStdString()))
    {
        BookmarkNode * const &node = m_lookupCache.get(url.toString().toStdString());
        if (node != nullptr)
        {
            BookmarkNode *nodeCopy = node;
            removeBookmark(nodeCopy);
            return;
        }
    }

    if (!m_nodeListFuture.isCanceled() && m_nodeListFuture.isRunning())
        m_nodeListFuture.waitForFinished();

    for (BookmarkNode *node : m_nodeList)
    {
        if (node->getType() != BookmarkNode::Bookmark)
            continue;

        // Bookmark URLs are unique, once match is found, remove bookmark and return
        if (node->getURL() == url)
        {
            removeBookmark(node);
            return;
        }
    }
}

void BookmarkManager::removeBookmark(BookmarkNode *item)
{
    if (!item || item == m_rootNode)
        return;

    // If node is a folder, add all sub-folders to the deletion queue. Otherwise just add the bookmark
    std::deque<BookmarkNode*> processQueue, deleteQueue;

    if (item->getType() == BookmarkNode::Folder)
        processQueue.push_back(item);
    else
        deleteQueue.push_back(item);

    while (!processQueue.empty())
    {
        BookmarkNode *node = processQueue.front();

        for (int i = 0; i < node->getNumChildren(); ++i)
        {
            BookmarkNode *child = node->getNode(i);
            if (child->getType() == BookmarkNode::Folder)
                processQueue.push_back(child);
            else if (child->m_type == BookmarkNode::Bookmark && m_lookupCache.has(item->m_url.toString().toStdString()))
                m_lookupCache.put(item->m_url.toString().toStdString(), nullptr);
        }

        deleteQueue.push_back(node);
        processQueue.pop_front();
    }

    // Iterate through deletion queue, emitting a signal for each folder to be removed from the database
    while (!deleteQueue.empty())
    {
        BookmarkNode *node = deleteQueue.back();
        BookmarkNode *parent = node->getParent();
        if (!parent)
            parent = m_rootNode;
        emit bookmarkDeleted(node->getUniqueId(), parent->getUniqueId(), node->getPosition());
        deleteQueue.pop_back();
    }

    if (item->m_type == BookmarkNode::Bookmark && m_lookupCache.has(item->m_url.toString().toStdString()))
        m_lookupCache.put(item->m_url.toString().toStdString(), nullptr);

    if (BookmarkNode *parent = item->getParent())
    {
        parent->removeNode(item);
        scheduleResetList();
    }
}

void BookmarkManager::setBookmarkName(BookmarkNode *bookmark, const QString &name)
{
    if (!bookmark || bookmark->getName() == name)
        return;

    bookmark->setName(name);

    emit bookmarkChanged(bookmark);
}

BookmarkNode *BookmarkManager::setBookmarkParent(BookmarkNode *bookmark, BookmarkNode *parent)
{
    if (!bookmark
            || !parent
            || bookmark == m_rootNode
            || bookmark->getParent() == parent
            || parent->getType() != BookmarkNode::Folder)
        return bookmark;

    // Don't allow user to set a bookmark folder's parent to its child. They
    // should use an intermediate or third folder instead
    if (bookmark->getType() == BookmarkNode::Folder)
    {
        BookmarkNode *temp = parent;
        while (temp->getParent())
        {
            BookmarkNode *tempParent = temp->getParent();
            if (tempParent == m_rootNode)
                break;
            if (tempParent == bookmark)
                return bookmark;

            temp = tempParent;
        }
    }

    BookmarkNode *oldParent = bookmark->getParent();
    for (auto it = oldParent->m_children.begin(); it != oldParent->m_children.end(); ++it)
    {
        if (it->get()->getUniqueId() == bookmark->getUniqueId())
        {
            parent->m_children.push_back(std::move(*it));
            it = oldParent->m_children.erase(it);
            break;
        }
    }

    bookmark = parent->getNode(parent->getNumChildren() - 1);
    bookmark->m_parent = parent;
    emit bookmarkChanged(bookmark);

    scheduleResetList();

    return bookmark;
}

void BookmarkManager::setBookmarkPosition(BookmarkNode *bookmark, int position)
{
    if (!bookmark || !bookmark->getParent())
        return;

    BookmarkNode *parent = bookmark->getParent();
    const int currentPos = bookmark->getPosition();

    // Make sure the position is valid
    if (position < 0 || position >= parent->getNumChildren() || position == currentPos)
        return;

    // Adjust position of node in parent's child list
    if (position > currentPos)
        ++position;
    static_cast<void>(parent->insertNode(std::make_unique<BookmarkNode>(std::move(*bookmark)), position));
    parent->removeNode(bookmark);

    bookmark = parent->getNode(position);
    emit bookmarkChanged(bookmark);

    scheduleResetList();
}

void BookmarkManager::setBookmarkShortcut(BookmarkNode *bookmark, const QString &shortcut)
{
    if (!bookmark || bookmark->getShortcut() == shortcut)
        return;

    bookmark->setShortcut(shortcut);

    emit bookmarkChanged(bookmark);
}

void BookmarkManager::setBookmarkURL(BookmarkNode *bookmark, const QUrl &url)
{
    if (!bookmark || bookmark->getURL().matches(url, QUrl::None))
        return;

    const QUrl oldUrl = bookmark->getURL();

    // Update cache if applicable
    const std::string oldUrlStr = oldUrl.toString().toStdString();
    if (m_lookupCache.has(oldUrlStr))
        m_lookupCache.put(oldUrlStr, nullptr);

    bookmark->setURL(url);
    bookmark->setIcon(m_faviconStore ? m_faviconStore->getFavicon(url) : QIcon());

    emit bookmarkChanged(bookmark);
}

void BookmarkManager::setRootNode(BookmarkNode *node)
{
    if (!node)
        return;

    m_rootNode = node;

    for (int i = 0; i < m_rootNode->getNumChildren(); ++i)
    {
        BookmarkNode *child = m_rootNode->getNode(i);
        if (child->getType() == BookmarkNode::Folder
                && child->getName().compare(QLatin1String("Bookmarks Bar")) == 0)
        {
            m_bookmarkBar = child;
            break;
        }
    }

    if (!m_bookmarkBar)
        m_bookmarkBar = m_rootNode;

    resetBookmarkList();
}

void BookmarkManager::scheduleResetList()
{
    if (m_canUpdateList)
        m_nodeListFuture = QtConcurrent::run(this, &BookmarkManager::resetBookmarkList);
}

void BookmarkManager::setCanUpdateList(bool value)
{
    m_canUpdateList = value;
    if (value)
        scheduleResetList();
}

void BookmarkManager::waitToFinishList()
{
    if (!m_nodeListFuture.isCanceled() && m_nodeListFuture.isRunning())
        m_nodeListFuture.waitForFinished();
}

void BookmarkManager::resetBookmarkList()
{
    std::lock_guard<std::mutex> _(m_mutex);

    int numBookmarks = 1;
    std::vector<BookmarkNode*> nodeList;

    std::deque<BookmarkNode*> queue;
    queue.push_back(m_rootNode);
    while (!queue.empty())
    {
        BookmarkNode *n = queue.front();

        for (auto &node : n->m_children)
        {
            ++numBookmarks;
            BookmarkNode *childNode = node.get();
            if (!childNode)
                continue;

            nodeList.push_back(childNode);

            if (childNode->getType() == BookmarkNode::Folder)
                queue.push_back(childNode);
        }

        queue.pop_front();
    }

    m_numBookmarks.store(numBookmarks);
    m_nodeList = std::move(nodeList);
    emit bookmarksChanged();
}
