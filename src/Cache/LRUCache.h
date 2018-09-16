#ifndef LRUCACHE_H
#define LRUCACHE_H

#include <list>
#include <stdexcept>
#include <unordered_map>

/**
 * @class LRUCache
 * @brief An implementation of a fixed-capacity least recently used cache
 */
template <typename KeyType, typename ValueType>
class LRUCache
{
    typedef typename std::pair<KeyType, ValueType> Node;
    typedef typename std::list<Node>::iterator ListIterator;

public:
    /// Constructs the LRU cache with a given maximum capacity
    explicit LRUCache(size_t maxSize) : m_maxSize(maxSize), m_list(), m_map() {}

    /// Returns true if the cache contains an item associated with the given key, false if else
    bool has(const KeyType &key) const
    {
        return m_map.find(key) != m_map.end();
    }

    /// Returns a const reference to the value associated with the given key.
    /// Throws an out_of_range exception if the key-value pair is not in the cache
    const ValueType &get(const KeyType &key)
    {
        auto it = m_map.find(key);
        if (it == m_map.end())
            throw std::out_of_range("LRUCache: key is not in the cache, cannot fetch value");

        // Move item to front of the list
        m_list.splice(m_list.begin(), m_list, it->second);

        return it->second->second;
    }

    /// Places the key-value pair into the front of the cache
    void put(const KeyType &key, const ValueType &value)
    {
        // Check if key is already in the map, if so, erase before reinsertion
        auto it = m_map.find(key);
        if (it != m_map.end())
        {
            m_list.erase(it->second);
            m_map.erase(it);
        }

        m_list.push_front({key, value});
        m_map[key] = m_list.begin();

        // Check if size is above capacity after insertion
        if (m_map.size() > m_maxSize)
        {
            // Remove LRU item
            auto lruIt = m_list.end();
            --lruIt;
            m_map.erase(lruIt->first);
            m_list.pop_back();
        }
    }

private:
    /// The maximum number of key-value pairs in the cache
    size_t m_maxSize;

    /// A doubly-linked list of key-value pairs
    std::list<Node> m_list;

    /// A hashmap of keys pointing to corresponding key-value pair iterators in the list
    std::unordered_map<KeyType, ListIterator> m_map;
};

#endif // LRUCACHE_H
