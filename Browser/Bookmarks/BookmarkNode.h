#ifndef BOOKMARKNODE_H
#define BOOKMARKNODE_H

#include "TreeNode.h"

#include <QDataStream>
#include <QIcon>
#include <QMetaType>
#include <QString>

/**
 * @class BookmarkNode
 * @brief Individual node that is a part of the Bookmarks tree. Each node
 *        can be a bookmark or a folder containing bookmarks and other folders
 */
class BookmarkNode : public TreeNode<BookmarkNode>
{
    friend class BookmarkManager;

public:
    /// List of the specific types of bookmark nodes. At the moment, there are only bookmarks and folders.
    enum NodeType { Folder = 0, Bookmark = 1 };

public:
    /// Default constructor. Node is made with no parent or data
    BookmarkNode();

    /// Constructs the node, specifying its type and assigning a name to the node
    BookmarkNode(NodeType type, const QString &name);

    /// Move constructor
    BookmarkNode(BookmarkNode &&other);

    /// Appends the given node to this node, returning a raw pointer to the child node
    BookmarkNode *appendNode(std::unique_ptr<BookmarkNode> node) override;

    /// Inserts the given node to this node at the given index, returning a raw pointer to the child node.
    /// If the index is invalid, this will simply append the node to the end
    BookmarkNode *insertNode(std::unique_ptr<BookmarkNode> node, int index) override;

    /// Returns the identifier of the node, if it is of type folder
    int getFolderId() const;

    /// Sets the identifier of the node
    void setFolderId(int id);

    /// Returns this node's type
    NodeType getType() const;

    /// Sets this node's type to the given value
    void setType(NodeType type);

    /// Returns the name of the bookmark node
    const QString &getName() const;

    /// Sets the name of the bookmark node to the given value
    void setName(const QString &name);

    /// Returns the URL of the node if its type is bookmark, returns an empty QString otherwise
    const QString &getURL() const;

    /// Sets the URL of the node
    void setURL(const QString &url);

    /// Returns the icon associated with the node
    const QIcon &getIcon() const;

    /// Sets the icon associated with the node
    void setIcon(const QIcon &icon);

protected:
    /// Name of the bookmark node
    QString m_name;

    /// URL associated with the node. Will be empty if the node type is Folder
    QString m_url;

    /// Icon associated with the node. For folders, will be standard folder icon. For bookmarks, will be favicon
    QIcon m_icon;

    /// Type of node
    NodeType m_type;

    /// Folder ID - If node is type folder, this refers to the node's own folder id.
    /// If the node is type bookmark, this refers to its parent folder id.
    int m_folderId;

    /// Pointer to the node's parent
    //BookmarkNode *m_parent;

    /// Vector of child nodes belonging to this node
    //std::vector< std::unique_ptr<BookmarkNode> > m_children;
};

Q_DECLARE_METATYPE(BookmarkNode::NodeType)
Q_DECLARE_METATYPE(BookmarkNode*)

QDataStream& operator<<(QDataStream &out, BookmarkNode *&node);
QDataStream& operator>>(QDataStream &in, BookmarkNode *&node);

#endif // BOOKMARKNODE_H
