/**
 * @author Vladimir Pavliv
 * @date 2025-03-06
 */

#ifndef HFT_SERVER_PRICEFEED_HPP
#define HFT_SERVER_PRICEFEED_HPP

#include "boost_types.hpp"
#include "config/config.hpp"
#include "server_bus.hpp"
#include "server_command.hpp"
#include "ticker_data.hpp"
#include "utils/market_utils.hpp"

namespace hft::server {

class PriceFeed {
public:
  PriceFeed(const MarketData &data, IoContext &ctx)
      : data_{data}, timer_{ctx}, rate_{Config::cfg.priceFeedRate} {

    ServerBus::systemBus.subscribe(ServerCommand::PriceFeedStart, [this]() { start(); });
    ServerBus::systemBus.subscribe(ServerCommand::PriceFeedStop, [this]() { stop(); });
  }

  void start() {
    timer_.expires_after(rate_);
    timer_.async_wait([this](BoostErrorRef ec) {
      if (ec) {
        return;
      }
      adjustPrices();
      start();
    });
  }

  void stop() { timer_.cancel(); }

private:
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
    ServerBus::marketBus.publish(Span<TickerPrice>(priceUpdates));
  }

private:
  const MarketData &data_;

  SteadyTimer timer_;
  Microseconds rate_;
};

} // namespace hft::server

#endif // HFT_SERVER_PRICEFEED_HPP