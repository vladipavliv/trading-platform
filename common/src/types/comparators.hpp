/**
 * @author Vladimir Pavliv
 * @date 2025-02-13
 */

#ifndef HFT_COMMON_MARKET_COMPARATORS_HPP
#define HFT_COMMON_MARKET_COMPARATORS_HPP

#include <algorithm>

#include "market_types.hpp"
#include "template_types.hpp"
#include "types.hpp"
#include "utils/template_utils.hpp"

namespace hft {

template <typename Type>
struct TickerCmp {
  bool operator()(const Type &left, const Type &right) { return left.ticker < right.ticker; }
};

template <typename Type>
struct TraderIdCmp {
  bool operator()(const Type &left, const Type &right) { return left.traderId < right.traderId; }
};

template <typename Type>
struct SizeCmp {
  bool operator()(const Type &left, const Type &right) { return getSize(left) < getSize(right); }

private:
  static size_t getSize(const Type &item) {
    if constexpr (std::is_pointer_v<Type> || is_smart_ptr<Type>::value) {
      return item->size();
    } else {
      return item.size();
    }
  }
};

} // namespace hft

#endif // HFT_COMMON_MARKET_COMPARATORS_HPP
