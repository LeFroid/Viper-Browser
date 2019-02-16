#ifndef TREENODE_H
#define TREENODE_H

#include <memory>
#include <utility>
#include <vector>

/**
 * @class TreeNode
 * @brief Implementation of a tree structure
 */
template <class T>
class TreeNode
{
public:
    /// Default constructor. Node is made with no parent or data
    TreeNode() = default;

    /// TreeNode destructor
    virtual ~TreeNode() = default;

    /// Appends the given node to this node, returning a raw pointer to the child node
    virtual T *appendNode(std::unique_ptr<T> node)
    {
        node->m_parent = dynamic_cast<T*>(this);
        T *nodePtr = node.get();
        m_children.push_back(std::move(node));
        return nodePtr;
    }

    /// Inserts the given node to this node at the given index, returning a raw pointer to the child node.
    /// If the index is invalid, this will simply append the node to the end
    virtual T *insertNode(std::unique_ptr<T> node, int index)
    {
        node->m_parent = dynamic_cast<T*>(this);
        T *nodePtr = node.get();

        if (index < 0 || index > static_cast<int>(m_children.size()))
        {
            m_children.push_back(std::move(node));
            return nodePtr;
        }

        m_children.insert(m_children.begin() + index, std::move(node));
        return nodePtr;
    }

    /// Attempts to remove the given node, returning true on success, false on failure
    bool removeNode(T *node)
    {
        if (!node)
            return false;

        for (auto it = m_children.cbegin(); it != m_children.cend(); ++it)
        {
            if (it->get() == node)
            {
                it = m_children.erase(it);
                return true;
            }
        }

        return false;
    }

    /// Attempts to remove the node at the given index, returning true on success, false on failure or invalid index.
    bool removeNodeAt(int index)
    {
        if (index < 0 || index >= static_cast<int>(m_children.size()))
            return false;
        auto it = m_children.cbegin() + index;
        m_children.erase(it);
        return true;
    }

    /// Returns the number of child nodes
    int getNumChildren() const
    {
        return static_cast<int>(m_children.size());
    }

    /// Returns a pointer to the node's parent, or a nullptr if this is the root node
    T *getParent() const
    {
        return m_parent;
    }

    /// Returns a pointer to the child node at the given index, or a nullptr if index is out of bounds
    T *getNode(int index) const
    {
        if (index < 0 || index >= static_cast<int>(m_children.size()))
            return nullptr;

        return m_children[index].get();
    }

protected:
    /// Pointer to the node's parent
    T *m_parent;

    /// Vector of child nodes belonging to this node
    std::vector< std::unique_ptr<T> > m_children;
};

#endif // TREENODE_H
