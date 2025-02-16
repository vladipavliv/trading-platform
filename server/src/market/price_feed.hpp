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
#include "server_types.hpp"
#include "utils/utils.hpp"

namespace hft::server::market {

class PriceFeed {
public:
  PriceFeed(ServerSink &sink) : mSink{sink}, mTimer{mSink.ctx()} {}

  void setCurrentPrices(std::vector<PriceUpdate> &&prices) { mPrices = std::move(prices); }

  void start() {
    mSpeed += 10;
    generate();
  }

  void stop() {
    mSpeed = mSpeed > 0 ? mSpeed - 10 : 0;
    mTimer.cancel();
  }

private:
  void generate() {
    if (mSpeed <= 0) {
      return;
    }
    mTimer.expires_after(mRate - MilliSeconds(mSpeed));
    mTimer.async_wait([this](BoostErrorRef ec) {
      if (!ec) {
        updatePrice();
        generate();
      }
    });
  }

  void updatePrice() {
    static auto cursor = mPrices.begin();
    if (cursor == mPrices.end()) {
      cursor = mPrices.begin();
    }
    cursor->price += utils::generateNumber(9000);
    mSink.networkSink.post(*cursor);
    cursor++;
  }

private:
  ServerSink &mSink;
  MilliSeconds mRate{MilliSeconds(FEED_RATE)};
  SteadyTimer mTimer;

  std::atomic<int> mSpeed{0};
  std::vector<PriceUpdate> mPrices;
};

} // namespace hft::server::market

#endif // HFT_SERVER_PRICEFEED_HPP
