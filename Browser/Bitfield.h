#ifndef BITFIELD_H
#define BITFIELD_H

#include <type_traits>

/// Templates to allow treating an enum class as a bitfield
template <typename E>
E operator &(const E &lhs, const E &rhs)
{
    using underlying_type_t = typename std::underlying_type<E>::type;
    return static_cast<E>(static_cast<underlying_type_t>(lhs) & static_cast<underlying_type_t>(rhs));
}
template <typename E>
E operator |(const E &lhs, const E &rhs)
{
    using underlying_type_t = typename std::underlying_type<E>::type;
    return static_cast<E>(static_cast<underlying_type_t>(lhs) | static_cast<underlying_type_t>(rhs));
}
template <typename E>
E operator ^(const E &lhs, const E &rhs)
{
    using underlying_type_t = typename std::underlying_type<E>::type;
    return static_cast<E>(static_cast<underlying_type_t>(lhs) ^ static_cast<underlying_type_t>(rhs));
}
template <typename E>
E operator ~(const E &val)
{
    using underlying_type_t = typename std::underlying_type<E>::type;
    return static_cast<E>(~ static_cast<underlying_type_t>(val));
}

#endif // BITFIELD_H
