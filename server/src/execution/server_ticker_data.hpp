/**
 * @author Vladimir Pavliv
 * @date 2025-02-13
 */

#ifndef HFT_SERVER_TICKERDATA_HPP
#define HFT_SERVER_TICKERDATA_HPP

#include <atomic>
#include <map>

#include <boost/unordered/unordered_flat_map.hpp>

#include "constants.hpp"
#include "domain_types.hpp"
#include "order_book.hpp"
#include "types.hpp"

namespace hft::server {

/**
 * @brief All the data in one place
 * @todo Add atomic flag to lock the book for rerouting
 */
class TickerData {
public:
  TickerData(ThreadId id, Price price) : threadId_{id} {}

  inline void setThreadId(ThreadId id) { threadId_.store(id, std::memory_order_release); }
  inline ThreadId getThreadId() const { return threadId_.load(std::memory_order_acquire); }

  OrderBook orderBook;

private:
  /**
   * @brief Even though threadId_ is frequently read by network threads
   * it wont be frequently written, only for rerouting, which is more of
   * an exception. So there is no need wasting whole cache line for that
   * More important to pack the data as tight as possible
   */
  std::atomic<ThreadId> threadId_;

  TickerData() = delete;
  TickerData(const TickerData &) = delete;
  TickerData &operator=(const TickerData &other) = delete;
  TickerData(TickerData &&) = delete;
  TickerData &operator=(TickerData &&other) = delete;
};
static_assert(sizeof(TickerData) == 64);

// class MarketData {};

using MarketData = boost::unordered_flat_map<Ticker, UPtr<TickerData>, TickerHash>;

} // namespace hft::server

#endif // HFT_SERVER_TICKERDATA_HPP
