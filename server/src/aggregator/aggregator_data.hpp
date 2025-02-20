/**
 * @author Vladimir Pavliv
 * @date 2025-02-19
 */

#ifndef HFT_SERVER_TICKERDATA_HPP
#define HFT_SERVER_TICKERDATA_HPP

#include <atomic>

#include "boost_types.hpp"
#include "market_types.hpp"
#include "order_book/flat_order_book.hpp"
#include "order_book/map_order_book.hpp"
#include "server_types.hpp"

namespace hft::server {

/**
 * @brief Need alignment cause when rebalancing would happen one thread could
 * increment the counter while another thread changes the threadId,
 * also price changes from separate thread
 * @todo Review later for improvement/reuse of empty space and data access patterns
 */
struct alignas(CACHE_LINE_SIZE) TickerData {
  OrderBook::UPtr orderBook;
  std::atomic<ThreadId> threadId;
  // TODO(): I can land a helicopter here
  alignas(CACHE_LINE_SIZE) std::atomic<size_t> eventCounter;
  alignas(CACHE_LINE_SIZE) mutable std::atomic<Price> currentPrice;

  using UPtr = std::unique_ptr<TickerData>;
};

using AggregatorData = std::unordered_map<Ticker, TickerData, TickerHash>;

} // namespace hft::server

#endif // HFT_SERVER_TICKERDATA_HPP