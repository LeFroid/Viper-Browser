#ifndef CLEARHISTORYOPTIONS_H
#define CLEARHISTORYOPTIONS_H

#include <cstdint>
#include <QMetaType>

/// History types that are chosen to be removed
enum class HistoryType : uint32_t
{
    None     = 0x0,    /// Nothing selected
    Browsing = 0x1,    /// Browsing and download history
    Search   = 0x2,    /// Form and search data
    Cache    = 0x4     /// Browser cache
};
constexpr enum HistoryType operator |(const enum HistoryType selfValue, const enum HistoryType inValue)
{
    return (enum HistoryType)(uint32_t(selfValue) | uint32_t(inValue));
}
constexpr enum HistoryType operator &(const enum HistoryType selfValue, const enum HistoryType inValue)
{
    return (enum HistoryType)(uint32_t(selfValue) & uint32_t(inValue));
}

Q_DECLARE_METATYPE(HistoryType)

#endif // CLEARHISTORYOPTIONS_H
