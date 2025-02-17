/**
 * @author Vladimir Pavliv
 * @date 2025-02-13
 */

#ifndef HFT_SERVER_PRICEFEED_HPP
#define HFT_SERVER_PRICEFEED_HPP

#include <atomic>
#include <spdlog/spdlog.h>
#include <vector>

#include "boost_types.hpp"
#include "market_types.hpp"
#include "prices_view.hpp"
#include "server_types.hpp"
#include "utils/rng.hpp"
#include "utils/utils.hpp"

namespace hft::server {

class PriceFeed {
public:
  PriceFeed(ServerSink &sink, PricesView prices)
      : mSink{sink}, mPrices{std::move(prices)}, mTimer{mSink.ctx()} {}

  void start() {
    mSpeed += 10;
    mInProgress.store(true);
    generate();
  }

  void stop() {
    mSpeed -= 10;
    mInProgress.store(false);
    mTimer.cancel();
  }

private:
  void generate() {
    if (!mInProgress) {
      return;
    }
    auto next = mRate - MilliSeconds(mSpeed);
    next = next < MilliSeconds(5) ? MilliSeconds(5) : next;
    mTimer.expires_after(next);
    mTimer.async_wait([this](BoostErrorRef ec) {
      if (!ec) {
        updatePrice();
        generate();
      }
    });
  }

  void updatePrice() {
    static auto cursor = mPrices.getPriceIterator();
    if (cursor.end()) {
      cursor.reset();
    }
    auto tickerPrice = *cursor;
    mPrices.setPrice({tickerPrice.ticker, utils::RNG::rng(900.0f)});
    mSink.networkSink.post(*cursor);
    cursor++;
  }

private:
  ServerSink &mSink;
  PricesView mPrices;

  MilliSeconds mRate{MilliSeconds(FEED_RATE)};
  SteadyTimer mTimer;

  std::atomic<int> mSpeed{0};
  std::atomic_bool mInProgress{false};
};

} // namespace hft::server

#endif // HFT_SERVER_PRICEFEED_HPP
