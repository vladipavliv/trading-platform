/**
 * @author Vladimir Pavliv
 * @date 2025-02-13
 */

#ifndef HFT_SERVER_MARKETDATA_HPP
#define HFT_SERVER_MARKETDATA_HPP

#include <boost/unordered/unordered_flat_map.hpp>

#include "constants.hpp"
#include "domain_types.hpp"
#include "execution/orderbook/flat_order_book.hpp"
#include "execution/orderbook/price_level_order_book.hpp"
#include "primitive_types.hpp"
#include "traits.hpp"

namespace hft::server {

/**
 * @brief All the data in one place
 * @todo Add atomic flag to lock the book for rerouting
 */
struct ALIGN_CL TickerData {
  explicit TickerData(ThreadId id) : workerId{id} {}

  TickerData(TickerData &&other) noexcept
      : workerId(other.workerId), orderBook(std::move(other.orderBook)) {}

  ThreadId workerId;
  ALIGN_CL mutable OrderBook orderBook;

private:
  TickerData() = delete;
  TickerData(const TickerData &) = delete;
  TickerData &operator=(const TickerData &other) = delete;
};

using MarketData = boost::unordered_flat_map<Ticker, TickerData, TickerHash>;

} // namespace hft::server

#endif // HFT_SERVER_TICKERDATA_HPP
