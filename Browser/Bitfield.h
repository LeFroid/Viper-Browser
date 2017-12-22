#ifndef BITFIELD_H
#define BITFIELD_H

#include <type_traits>

template<typename E>
struct EnableBitfield
{
    static constexpr bool enabled = false;
};

/// Templates to allow treating an enum class as a bitfield
template <typename E>
typename std::enable_if<EnableBitfield<E>::enabled, E>::type
operator &(E lhs, E rhs)
{
    using underlying_type_t = typename std::underlying_type<E>::type;
    return static_cast<E>(static_cast<underlying_type_t>(lhs) & static_cast<underlying_type_t>(rhs));
}
template <typename E>
typename std::enable_if<EnableBitfield<E>::enabled, E>::type
operator &=(E &lhs, E rhs)
{
    using underlying_type_t = typename std::underlying_type<E>::type;
    lhs = static_cast<E>(static_cast<underlying_type_t>(lhs) & static_cast<underlying_type_t>(rhs));
    return lhs;
}
template <typename E>
typename std::enable_if<EnableBitfield<E>::enabled, E>::type
operator |(E lhs, E rhs)
{
    using underlying_type_t = typename std::underlying_type<E>::type;
    return static_cast<E>(static_cast<underlying_type_t>(lhs) | static_cast<underlying_type_t>(rhs));
}
template <typename E>
typename std::enable_if<EnableBitfield<E>::enabled, E>::type
operator |=(E &lhs, E rhs)
{
    using underlying_type_t = typename std::underlying_type<E>::type;
    lhs = static_cast<E>(static_cast<underlying_type_t>(lhs) | static_cast<underlying_type_t>(rhs));
    return lhs;
}
template <typename E>
typename std::enable_if<EnableBitfield<E>::enabled, E>::type
operator ^(const E lhs, E rhs)
{
    using underlying_type_t = typename std::underlying_type<E>::type;
    return static_cast<E>(static_cast<underlying_type_t>(lhs) ^ static_cast<underlying_type_t>(rhs));
}
template <typename E>
typename std::enable_if<EnableBitfield<E>::enabled, E>::type
operator ~(E val)
{
    using underlying_type_t = typename std::underlying_type<E>::type;
    return static_cast<E>(~ static_cast<underlying_type_t>(val));
}

#endif // BITFIELD_H
