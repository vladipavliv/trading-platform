/**
 * @author Vladimir Pavliv
 * @date 2025-02-13
 */

#ifndef HFT_SERVER_MARKETDATA_HPP
#define HFT_SERVER_MARKETDATA_HPP

#include <atomic>

#include <boost/unordered/unordered_flat_map.hpp>

#include "constants.hpp"
#include "domain_types.hpp"
#include "order_book.hpp"
#include "primitive_types.hpp"

namespace hft::server {

/**
 * @brief All the data in one place
 * @todo Add atomic flag to lock the book for rerouting
 */
class alignas(CACHE_LINE_SIZE) TickerData {
  std::atomic<ThreadId> threadId_;

public:
  explicit TickerData(ThreadId id) : threadId_{id} {}

  inline void setThreadId(ThreadId id) { threadId_.store(id, std::memory_order_release); }
  inline ThreadId getThreadId() const { return threadId_.load(std::memory_order_acquire); }

  mutable OrderBook orderBook;

private:
  TickerData() = delete;
  TickerData(const TickerData &) = delete;
  TickerData &operator=(const TickerData &other) = delete;
};

using MarketData = boost::unordered_flat_map<Ticker, UPtr<TickerData>, TickerHash>;

} // namespace hft::server

#endif // HFT_SERVER_TICKERDATA_HPP
