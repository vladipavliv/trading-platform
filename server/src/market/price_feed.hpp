/**
 * @file
 * @brief
 *
 * @author Vladimir Pavliv
 * @date 2025-02-13
 */

#ifndef HFT_SERVER_PRICEFEED_HPP
#define HFT_SERVER_PRICEFEED_HPP

#include <atomic>
#include <spdlog/spdlog.h>

#include "boost_types.hpp"
#include "market_types.hpp"
#include "server_types.hpp"
#include "utils/utils.hpp"

namespace hft::server::market {

class PriceFeed {
public:
  PriceFeed(ServerSink &sink) : mSink{sink}, mTimer{mSink.ctx()} {}

  void start() {
    mGenerating = true;
    generate();
  }

  void stop() {
    mGenerating = false;
    mTimer.cancel();
  }

private:
  void generate() {
    if (!mGenerating) {
      return;
    }
    mTimer.expires_after(mRate);
    mTimer.async_wait([this](BoostErrorRef ec) {
      if (!ec) {
        updatePrices();
        generate();
      }
    });
  }

  void updatePrices() {
    PriceUpdate update = utils::generatePriceUpdate();
    mSink.networkSink.post(update);
  }

private:
  ServerSink &mSink;
  MilliSeconds mRate{MilliSeconds(10)};

  std::atomic_bool mGenerating;
  SteadyTimer mTimer;
};

} // namespace hft::server::market

#endif // HFT_SERVER_PRICEFEED_HPP
