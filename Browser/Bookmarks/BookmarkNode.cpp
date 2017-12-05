#include "BookmarkNode.h"
#include <algorithm>
#include <utility>
#include <cstdint>

BookmarkNode::BookmarkNode() :
    TreeNode<BookmarkNode>(),
    m_name(),
    m_url(),
    m_icon(),
    m_type(BookmarkNode::Bookmark),
    m_folderId(0)
{
}

BookmarkNode::BookmarkNode(BookmarkNode::NodeType type, const QString &name) :
    TreeNode<BookmarkNode>(),
    m_name(name),
    m_url(),
    m_icon(),
    m_type(type),
    m_folderId(0)
{
}

BookmarkNode::BookmarkNode(BookmarkNode &&other)
{
    m_name = other.m_name;
    m_url = other.m_url;
    m_type = other.m_type;
    m_folderId = other.m_folderId;
    m_parent = other.m_parent;
    m_icon = std::move(other.m_icon);
    m_children = std::move(other.m_children);
}

BookmarkNode *BookmarkNode::appendNode(std::unique_ptr<BookmarkNode> node)
{
    BookmarkNode *nodePtr = TreeNode<BookmarkNode>::appendNode(std::move(node));
    if (nodePtr->getType() != BookmarkNode::Folder)
        nodePtr->m_folderId = m_folderId;
    return nodePtr;
}

BookmarkNode *BookmarkNode::insertNode(std::unique_ptr<BookmarkNode> node, int index)
{
    BookmarkNode *nodePtr = TreeNode<BookmarkNode>::insertNode(std::move(node), index);
    if (nodePtr->getType() != BookmarkNode::Folder)
        nodePtr->m_folderId = m_folderId;
    return nodePtr;
}

int BookmarkNode::getFolderId() const
{
    return m_folderId;
}

void BookmarkNode::setFolderId(int id)
{
    m_folderId = id;
}

BookmarkNode::NodeType BookmarkNode::getType() const
{
    return m_type;
}

void BookmarkNode::setType(BookmarkNode::NodeType type)
{
    m_type = type;
}

const QString &BookmarkNode::getName() const
{
    return m_name;
}

void BookmarkNode::setName(const QString &name)
{
    m_name = name;
}

const QString &BookmarkNode::getURL() const
{
    return m_url;
}

void BookmarkNode::setURL(const QString &url)
{
    m_url = url;
}

const QIcon &BookmarkNode::getIcon() const
{
    return m_icon;
}

void BookmarkNode::setIcon(const QIcon &icon)
{
    m_icon = icon;
}

QDataStream& operator<<(QDataStream &out, BookmarkNode *&node)
{
    std::intptr_t ptr = reinterpret_cast<std::intptr_t>(node);
    out.writeRawData((const char*)&ptr, sizeof(std::intptr_t));
    return out;
}

QDataStream& operator>>(QDataStream &in, BookmarkNode *&node)
{
    std::intptr_t ptr;
    in.readRawData((char*)&ptr, sizeof(std::intptr_t));
    node = reinterpret_cast<BookmarkNode*>(ptr);
    return in;
}
