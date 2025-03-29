/**
 * @author Vladimir Pavliv
 * @date 2025-03-06
 */

#ifndef HFT_SERVER_PRICEFEED_HPP
#define HFT_SERVER_PRICEFEED_HPP

#include "boost_types.hpp"
#include "bus/bus.hpp"
#include "config/config.hpp"
#include "server_command.hpp"
#include "ticker_data.hpp"
#include "utils/market_utils.hpp"

namespace hft::server {

/**
 * @brief Simple price fluctuator that iterates over the prices at a given rate,
 * changes the price within a given delta, and posts the price update over a market bus
 * @details Accepts a const reference to a market data, which has the price as a mutable atomic
 * so it can be changed. Saves memory having one global market data storage.
 */
class PriceFeed {
public:
  PriceFeed(Bus &bus, const MarketData &data)
      : bus_{bus}, data_{data}, timer_{bus_.systemBus.systemIoCtx},
        rate_{Config::cfg.priceFeedRate} {
    bus_.systemBus.subscribe(ServerCommand::PriceFeedStart, [this]() { start(); });
    bus_.systemBus.subscribe(ServerCommand::PriceFeedStop, [this]() { stop(); });
  }

  void start() {
    Logger::monitorLogger->info("Start broadcasting price changes");
    broadcasting_ = true;
    schedulePriceChange();
  }

  void stop() {
    Logger::monitorLogger->info("Stop broadcasting price changes");
    timer_.cancel();
    broadcasting_ = false;
  }

private:
  void schedulePriceChange() {
    if (!broadcasting_) {
      return;
    }
    timer_.expires_after(rate_);
    timer_.async_wait([this](CRef<BoostError> ec) {
      if (ec) {
        return;
      }
      adjustPrices();
      schedulePriceChange();
    });
  }

  void adjustPrices() {
    static auto cursor = data_.begin();
    std::vector<TickerPrice> priceUpdates;
    priceUpdates.reserve(PRICE_UPDATE_CHUNK);
    for (int i = 0; i < PRICE_UPDATE_CHUNK; ++i) {
      if (cursor == data_.end()) {
        cursor = data_.begin();
      }
      const auto &tickerData = *cursor++;
      Price newPrice = utils::fluctuateThePrice(tickerData.second->getPrice());
      tickerData.second->setPrice(newPrice);
      priceUpdates.emplace_back(TickerPrice{tickerData.first, newPrice});
    }
    spdlog::trace([&priceUpdates] { return utils::toString(priceUpdates); }());
    bus_.marketBus.post(Span<TickerPrice>(priceUpdates));
  }

private:
  Bus &bus_;
  const MarketData &data_;

  SteadyTimer timer_;
  Microseconds rate_;

  std::atomic_bool broadcasting_{false};
};

} // namespace hft::server

#endif // HFT_SERVER_PRICEFEED_HPP
