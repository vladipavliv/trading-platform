/**
 * @author Vladimir Pavliv
 * @date 2025-03-06
 */

#ifndef HFT_SERVER_PRICEFEED_HPP
#define HFT_SERVER_PRICEFEED_HPP

#include "boost_types.hpp"
#include "config/server_config.hpp"
#include "server_command.hpp"
#include "server_ticker_data.hpp"
#include "server_types.hpp"
#include "utils/market_utils.hpp"

namespace hft::server {

/**
 * @brief Randomly changes the prices at a given rate and posts the changes over the market bus
 * @todo Do proper price changing algorighm
 */
class PriceFeed {
public:
  PriceFeed(Bus &bus, const MarketData &data)
      : bus_{bus}, data_{data}, timer_{bus_.systemCtx()}, rate_{ServerConfig::cfg.priceFeedRate} {
    bus_.systemBus.subscribe<ServerCommand>(ServerCommand::PriceFeedStart, [this] { start(); });
    bus_.systemBus.subscribe<ServerCommand>(ServerCommand::PriceFeedStop, [this] { stop(); });
  }

  void start() {
    LOG_INFO_SYSTEM("Start broadcasting price changes");
    broadcasting_ = true;
    schedulePriceChange();
  }

  void stop() {
    LOG_INFO_SYSTEM("Stop broadcasting price changes");
    timer_.cancel();
    broadcasting_ = false;
  }

private:
  void schedulePriceChange() {
    if (!broadcasting_) {
      return;
    }
    timer_.expires_after(rate_);
    timer_.async_wait([this](BoostErrorCode ec) {
      if (ec) {
        return;
      }
      adjustPrices();
      schedulePriceChange();
    });
  }

  void adjustPrices() {
    static auto cursor = data_.begin();
    if (cursor == data_.end()) {
      cursor = data_.begin();
    }
    const auto &tickerData = *cursor++;
    const Price newPrice = utils::fluctuateThePrice(tickerData.second->getPrice());
    tickerData.second->setPrice(newPrice);
    bus_.marketBus.post(TickerPrice{tickerData.first, newPrice});
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
