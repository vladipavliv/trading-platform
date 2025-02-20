/**
 * @author Vladimir Pavliv
 * @date 2025-02-13
 */

#ifndef HFT_TRADER_TRADING_STATS_HPP
#define HFT_TRADER_TRADING_STATS_HPP

#include "boost_types.hpp"
#include "types.hpp"
#include "utils/rng.hpp"

namespace hft::trader {

struct alignas(64) TradingStats {
  std::atomic_uint64_t balance{0};
  Padding<size_t> p;
  std::atomic_uint64_t ordersClosed{0};
};

} // namespace hft::trader

#endif // HFT_TRADER_TRADING_STATS_HPP