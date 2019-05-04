#include "BookmarkNode.h"
#include <algorithm>
#include <utility>
#include <cstdint>

BookmarkNode::BookmarkNode() :
    TreeNode<BookmarkNode>(),
    m_id(0),
    m_name(),
    m_url(),
    m_icon(),
    m_shortcut(),
    m_type(BookmarkNode::Bookmark)
{
}

BookmarkNode::BookmarkNode(BookmarkNode::NodeType type, const QString &name) :
    TreeNode<BookmarkNode>(),
    m_id(0),
    m_name(name),
    m_url(),
    m_icon(),
    m_shortcut(),
    m_type(type)
{
}

BookmarkNode::BookmarkNode(BookmarkNode &&other) noexcept
{
    m_id = other.m_id;
    m_name = other.m_name;
    m_url = other.m_url;
    m_shortcut = other.m_shortcut;
    m_type = other.m_type;
    m_parent = other.m_parent;
    m_icon = std::move(other.m_icon);
    m_children = std::move(other.m_children);
}

int BookmarkNode::getPosition() const
{
    if (!m_parent)
        return 0;

    for (int i = 0; i < m_parent->getNumChildren(); ++i)
    {
        if (m_parent->getNode(i) == this)
            return i;
    }

    return m_parent->getNumChildren() - 1;
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

const QString &BookmarkNode::getShortcut() const
{
    return m_shortcut;
}

void BookmarkNode::setShortcut(const QString &shortcut)
{
    m_shortcut = shortcut;
}

const QUrl &BookmarkNode::getURL() const
{
    return m_url;
}

void BookmarkNode::setURL(const QUrl &url)
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

int BookmarkNode::getUniqueId() const
{
    return m_id;
}

void BookmarkNode::setUniqueId(int id)
{
    m_id = id;
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
