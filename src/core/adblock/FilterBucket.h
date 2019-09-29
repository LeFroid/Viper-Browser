#ifndef FILTERBUCKET_H
#define FILTERBUCKET_H

#include "AdBlockFilter.h"
#include <cstdint>

/*
  35 // fedcba9876543210
  36 //       |    | |||
  37 //       |    | |||
  38 //       |    | |||
  39 //       |    | |||
  40 //       |    | ||+---- bit    0: [BlockAction | AllowAction]
  41 //       |    | |+----- bit    1: `important`
  42 //       |    | +------ bit 2- 3: party [0 - 3]
  43 //       |    +-------- bit 4- 8: type [0 - 31]
  44 //       +------------- bit 9-15: unused
*/

namespace adblock
{

using filter_mask_t = uint16_t;
/*
class FilterMask
{
private:
    filter_mask_t m_value;

public:
    constexpr FilterMask() noexcept : m_value() {}
    constexpr FilterMask(filter_mask_t value) noexcept : m_value(value) {}

    FilterMask(const FilterMask &) = default;
    FilterMask(FilterMask &&) = default;

    FilterMask &operator=(const FilterMask &) = default;
    FilterMask &operator=(FilterMask &&) = default;

    constexpr const filter_mask_t &get() const noexcept { return m_value; }
};
*/
class FilterBucket
{
public:
    FilterBucket() = default;
};

}

#endif // FILTERBUCKET_H
