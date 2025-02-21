/**
 * @author Vladimir Pavliv
 * @date 2025-02-13
 */

#ifndef HFT_COMMON_MARKET_COMPARATORS_HPP
#define HFT_COMMON_MARKET_COMPARATORS_HPP

#include <algorithm>

#include "market_types.hpp"
#include "types.hpp"

namespace hft {

template <typename Type>
struct TickerCmp {
  bool operator()(const Type &left, const Type &right) { return left.ticker < right.ticker; }
};

template <typename Type>
struct TraderIdCmp {
  bool operator()(const Type &left, const Type &right) { return left.traderId < right.traderId; }
};

} // namespace hft

#endif // HFT_COMMON_MARKET_COMPARATORS_HPP
