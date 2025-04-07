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
#include "market_types.hpp"
#include "order_book.hpp"
#include "types.hpp"

namespace hft::server {

/**
 * @brief All the data in one place
 * @todo Add atomic flag to lock the book for rerouting
 */
struct TickerData {
  using UPtr = std::unique_ptr<TickerData>;

  TickerData(ThreadId id, Price price) : threadId_{id}, price_{price} {}

  inline void setThreadId(ThreadId id) { threadId_.store(id, std::memory_order_release); }
  inline ThreadId getThreadId() const { return threadId_.load(std::memory_order_acquire); }

  inline void setPrice(Price price) { price_.store(price, std::memory_order_release); }
  inline Price getPrice() const { return price_.load(std::memory_order_acquire); }

  OrderBook orderBook;

private:
  alignas(CACHE_LINE_SIZE) std::atomic<ThreadId> threadId_;
  alignas(CACHE_LINE_SIZE) mutable std::atomic<Price> price_;

  TickerData() = delete;
  TickerData(const TickerData &) = delete;
  TickerData &operator=(const TickerData &other) = delete;
  TickerData(TickerData &&) = delete;
  TickerData &operator=(TickerData &&other) = delete;
};

using MarketData = boost::unordered_flat_map<Ticker, TickerData::UPtr, TickerHash>;

} // namespace hft::server

#endif // HFT_SERVER_TICKERDATA_HPP
