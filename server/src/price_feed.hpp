/**
 * @author Vladimir Pavliv
 * @date 2025-03-06
 */

#ifndef HFT_SERVER_PRICEFEED_HPP
#define HFT_SERVER_PRICEFEED_HPP

#include "boost_types.hpp"
#include "config/config.hpp"
#include "event_bus.hpp"
#include "server_command.hpp"
#include "ticker_data.hpp"

namespace hft::server {

class PriceFeed {
public:
  PriceFeed(const MarketData &data, IoContext &ctx)
      : data_{data}, timer_{ctx}, rate_{Config::cfg.priceFeedRateUs} {
    EventBus::bus().subscribe<ServerCommand>([this](Span<ServerCommand> commands) {
      switch (commands.front()) {
      case ServerCommand::PriceFeedStart:
        start();
        break;
      case ServerCommand::PriceFeedStop:
      case ServerCommand::Shutdown:
        stop();
      default:
        break;
      }
    });
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
      Price newPrice = utils::getLinuxTimestamp() % 777;
      tickerData.second->setPrice(newPrice);
      priceUpdates.emplace_back(TickerPrice{tickerData.first, newPrice});
    }
    spdlog::trace([&priceUpdates] { return utils::toString(priceUpdates); }());
    EventBus::bus().publish(Span<TickerPrice>(priceUpdates));
  }

private:
  const MarketData &data_;

  SteadyTimer timer_;
  Microseconds rate_;
};

} // namespace hft::server

#endif // HFT_SERVER_PRICEFEED_HPP