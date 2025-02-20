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

struct TradingStats {
  double balance{0};
  size_t operations{0};
  size_t rttSum{0};
  size_t rttBest{std::numeric_limits<size_t>::max()};
  size_t rttWorst{0};
  size_t rttSpikes{0};
};

} // namespace hft::trader

#endif // HFT_TRADER_TRADING_STATS_HPP