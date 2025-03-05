/**
 * @author Vladimir Pavliv
 * @date 2025-02-13
 */

#ifndef HFT_SERVER_TICKERDATA_HPP
#define HFT_SERVER_TICKERDATA_HPP

#include <atomic>
#include <map>
// TODO #include <boost/container/unordered_flat_map.hpp>

#include "market_types.hpp"
#include "order_book.hpp"
#include "types.hpp"

namespace hft::server {

struct TickerData {
  using UPtr = std::unique_ptr<TickerData>;

  TickerData(ThreadId id, Price currentPrice) : threadId{id}, price{currentPrice} {}

  TickerData() = delete;
  TickerData(const TickerData &) = delete;
  TickerData &operator=(const TickerData &other) = delete;
  TickerData(TickerData &&) = delete;
  TickerData &operator=(TickerData &&other) = delete;

  inline void setThreadId(ThreadId id) { threadId.store(id, std::memory_order_release); }
  inline ThreadId getThreadId() const { return threadId.load(std::memory_order_acquire); }

  inline void setPrice(Price currentPrice) { price.store(currentPrice, std::memory_order_release); }
  inline Price getPrice() const { return price.load(std::memory_order_acquire); }

  OrderBook orderBook;

private:
  alignas(CACHE_LINE_SIZE) std::atomic<ThreadId> threadId;
  alignas(CACHE_LINE_SIZE) mutable std::atomic<Price> price;
};

using MarketData = std::unordered_map<Ticker, TickerData::UPtr, TickerHash>;

} // namespace hft::server

#endif // HFT_SERVER_TICKERDATA_HPP