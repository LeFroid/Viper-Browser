#ifndef BOOKMARKNODE_H
#define BOOKMARKNODE_H

#include "SQLiteWrapper.h"
#include "../database/bindings/QtSQLite.h"

#include "TreeNode.h"

#include <QDataStream>
#include <QIcon>
#include <QMetaType>
#include <QString>
#include <QUrl>

/**
 * @class BookmarkNode
 * @brief Individual node that is a part of the Bookmarks tree. Each node
 *        can be a bookmark or a folder containing bookmarks and other folders
 * @ingroup Bookmarks
 */
class BookmarkNode : public TreeNode<BookmarkNode> , public sqlite::Row
{
    friend class BookmarkManager;
    friend class BookmarkStore;

public:
    /// List of the specific types of bookmark nodes. At the moment, there are only bookmarks and folders.
    enum NodeType { Folder = 0, Bookmark = 1 };

public:
    /// Default constructor. Node is made with no parent or data
    BookmarkNode();

    /// Constructs the node, specifying its type and assigning a name to the node
    BookmarkNode(NodeType type, const QString &name);

    /// Move constructor
    BookmarkNode(BookmarkNode &&other) noexcept;

    /// Returns the position of the bookmark node in relation to its siblings
    int getPosition() const;

    /// Returns this node's type
    NodeType getType() const;

    /// Returns the name of the bookmark node
    const QString &getName() const;

    /// Returns the shortcut used to load the bookmark
    const QString &getShortcut() const;

    /// Returns the URL of the node if its type is bookmark, returns an empty QString otherwise
    const QUrl &getURL() const;

    /// Returns the icon associated with the node
    const QIcon &getIcon() const;

    /// Sets the icon associated with the node
    void setIcon(const QIcon &icon);

protected:
    /// Returns the unique identifier of the node
    int getUniqueId() const;

    /// Sets the unique identifier of the node
    void setUniqueId(int id);

    /// Sets the name of the bookmark node to the given value
    void setName(const QString &name);

    /// Sets the shortcut that can be used to load the bookmark
    void setShortcut(const QString &shortcut);

    /// Sets this node's type to the given value
    void setType(NodeType type);

    /// Sets the URL of the node
    void setURL(const QUrl &url);

protected:
    /// Unique identifier of the node as stored in the database
    int m_id;

    /// Name of the bookmark node
    QString m_name;

    /// URL associated with the node. Will be empty if the node type is Folder
    QUrl m_url;

    /// Icon associated with the node. For folders, will be standard folder icon. For bookmarks, will be favicon
    QIcon m_icon;

    /// Shortcut to load the bookmark through the \ref URLLineEdit
    QString m_shortcut;

    /// Type of node
    NodeType m_type;

public:
    /// Writes the bookmark node into the prepared statement
    void marshal(sqlite::PreparedStatement &stmt) const override
    {
        int parentId = -1;
        if (m_parent)
            parentId = m_parent->getUniqueId();

        stmt << m_id
             << parentId
             << static_cast<int>(m_type)
             << m_name
             << m_url
             << m_shortcut
             << getPosition();
    }

    /// Not used
    void unmarshal(sqlite::PreparedStatement &/*stmt*/) override {}
};

Q_DECLARE_METATYPE(BookmarkNode::NodeType)
Q_DECLARE_METATYPE(BookmarkNode*)

QDataStream& operator<<(QDataStream &out, BookmarkNode *&node);
QDataStream& operator>>(QDataStream &in, BookmarkNode *&node);

#endif // BOOKMARKNODE_H
