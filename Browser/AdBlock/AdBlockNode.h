#ifndef ADBLOCKNODE_H
#define ADBLOCKNODE_H

#include "TreeNode.h"
#include <vector>
#include <QChar>

namespace AdBlock
{
    class AdBlockFilter;

    /**
     * @class AdBlockNode
     * @brief A tree node containing one or more \ref AdBlockFilter objects and a character associated with the node.
     *        Used for searching for filters that are of category StringContains during network requests.
     *        Example structure: root (no char) -> 'a' (no filters) -> 'd' (filters matching string 'ad' or regexp with 'ad*' etc), 'c' (no filters) -> child 0 of c, child 1 of c
     */
    class AdBlockNode : public TreeNode<AdBlockNode>
    {
    public:
        /// Default constructor
        AdBlockNode();

    private:
        /// Character associated with the node. Used for tree traversal
        QChar m_char;

        /// Container of filters associated with the node
        std::vector<AdBlockFilter*> m_filters;
    };
}

#endif // ADBLOCKNODE_H
