/**
 * @author Vladimir Pavliv
 * @date 2025-02-13
 */

#ifndef HFT_CLIENT_TICKERDATA_HPP
#define HFT_CLIENT_TICKERDATA_HPP

#include <atomic>

#include <boost/unordered/unordered_flat_map.hpp>

#include "constants.hpp"
#include "domain_types.hpp"
#include "types.hpp"

namespace hft::client {

/**
 * @brief Holds only the price for now
 * @todo Later on would track all the opened orders
 */
struct TickerData {
  explicit TickerData(Price price) : price_{price} {}

  inline void setPrice(Price price) { price_.store(price, std::memory_order_release); }
  inline Price getPrice() const { return price_.load(std::memory_order_acquire); }

private:
  alignas(CACHE_LINE_SIZE) mutable std::atomic<Price> price_;

  TickerData() = delete;
  TickerData(const TickerData &) = delete;
  TickerData &operator=(const TickerData &other) = delete;
  TickerData(TickerData &&) = delete;
  TickerData &operator=(TickerData &&other) = delete;
};

using MarketData = boost::unordered_flat_map<Ticker, UPtr<TickerData>, TickerHash>;

} // namespace hft::client

#endif // HFT_CLIENT_TICKERDATA_HPP
