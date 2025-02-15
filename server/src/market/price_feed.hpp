/**
 * @file
 * @brief
 *
 * @author Vladimir Pavliv
 * @date 2025-02-13
 */

#ifndef HFT_SERVER_PRICEFEED_HPP
#define HFT_SERVER_PRICEFEED_HPP

#include <spdlog/spdlog.h>

#include "boost_types.hpp"
#include "market_types.hpp"
#include "server_types.hpp"
#include "utils/utils.hpp"

namespace hft::server::market {

class PriceFeed {
public:
  PriceFeed(ServerSink &sink) : mSink{sink}, mTimer{mSink.ctx()} {}

  void start(MilliSeconds rate) {
    mRate = rate;
    expire();
  }
  void stop() { mTimer.cancel(); }

private:
  void expire() {
    mTimer.expires_after(mRate);
    mTimer.async_wait([this](BoostErrorRef ec) {
      if (ec) {
        spdlog::error("Failed generating price feed {}", ec.message());
      }
      updatePrices();
      expire();
    });
  }

  void updatePrices() {
    PriceUpdate update = utils::generatePriceUpdate();
    mSink.networkSink.post(update);
  }

private:
  ServerSink &mSink;
  MilliSeconds mRate;
  SteadyTimer mTimer;
};

} // namespace hft::server::market

#endif // HFT_SERVER_PRICEFEED_HPP
