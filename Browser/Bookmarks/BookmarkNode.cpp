#include "BookmarkNode.h"
#include <algorithm>
#include <utility>
#include <cstdint>

#include <QDebug>

BookmarkNode::BookmarkNode() :
    m_name(),
    m_url(),
    m_icon(),
    m_type(BookmarkNode::Bookmark),
    m_folderId(0),
    m_parent(nullptr),
    m_children()
{
}

BookmarkNode::BookmarkNode(BookmarkNode::NodeType type, const QString &name) :
    m_name(name),
    m_url(),
    m_icon(),
    m_type(type),
    m_folderId(0),
    m_parent(nullptr),
    m_children()
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
    node->m_parent = this;
    if (node->getType() != BookmarkNode::Folder)
        node->m_folderId = m_folderId;
    BookmarkNode *nodePtr = node.get();
    m_children.push_back(std::move(node));
    return nodePtr;
}

BookmarkNode *BookmarkNode::insertNode(std::unique_ptr<BookmarkNode> node, int index)
{
    node->m_parent = this;
    if (node->getType() != BookmarkNode::Folder)
        node->m_folderId = m_folderId;
    BookmarkNode *nodePtr = node.get();

    if (index < 0 || index > static_cast<int>(m_children.size()))
    {
        m_children.push_back(std::move(node));
        return nodePtr;
    }

    m_children.insert(m_children.begin() + index, std::move(node));
    return nodePtr;
}

bool BookmarkNode::removeNode(BookmarkNode *node)
{
    if (!node)
        return false;

    for (auto it = m_children.cbegin(); it != m_children.cend(); ++it)
    {
        if (it->get() == node)
        {
            m_children.erase(it);
            return true;
        }
    }

    return false;
}

bool BookmarkNode::removeNode(int index)
{
    if (index < 0 || index >= static_cast<int>(m_children.size()))
        return false;
    auto it = m_children.cbegin() + index;
    m_children.erase(it);
    return true;
}

int BookmarkNode::getNumChildren() const
{
    return static_cast<int>(m_children.size());
}

BookmarkNode *BookmarkNode::getParent()
{
    return m_parent;
}

BookmarkNode *BookmarkNode::getNode(int index)
{
    if (index < 0 || index >= static_cast<int>(m_children.size()))
        return nullptr;

    return m_children[index].get();
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
