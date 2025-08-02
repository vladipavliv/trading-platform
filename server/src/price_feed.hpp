/**
 * @author Vladimir Pavliv
 * @date 2025-03-06
 */

#ifndef HFT_SERVER_PRICEFEED_HPP
#define HFT_SERVER_PRICEFEED_HPP

#include "boost_types.hpp"
#include "commands/server_command.hpp"
#include "config/server_config.hpp"
#include "execution/server_ticker_data.hpp"
#include "server_types.hpp"
#include "utils/rng.hpp"

namespace hft::server {

/**
 * @brief Changes prices smoothly during random time periods
 */
class PriceFeed {
  /**
   * @brief Local price trend for the given ticker
   */
  struct Fluctuation {
    static constexpr size_t MIN_DURATION_US = // min period of 100ms
        100000;
    static constexpr size_t MAX_DURATION_US = // max period of 5s
        5000000;
    static constexpr double MAX_RATE_US = // 10000% per day in us
        100 / 86400.0 / 1000.0 / 1000.0;

    Fluctuation(CRef<Ticker> ticker, CRef<TickerData> data)
        : ticker{ticker}, data{data}, price{static_cast<double>(data.getPrice())},
          drift{utils::RNG::generate<double>(-MAX_RATE_US / 365, MAX_RATE_US / 365)} {
      randomize(utils::getTimestamp());
    };

    void randomize(Timestamp now) {
      using namespace utils;

      rate = price * RNG::generate<double>(-MAX_RATE_US + drift, MAX_RATE_US + drift);
      duration = RNG::generate<size_t>(MIN_DURATION_US, MAX_DURATION_US);
      lastUpdate = now;
      LOG_DEBUG("{} price:{} rate:{} duration:{}", // format
                utils::toString(ticker), price, rate, duration);
    }

    bool update(Timestamp timeStamp) {
      using namespace utils;

      auto timeDelta = timeStamp - lastUpdate;
      if (duration > timeDelta) {
        duration -= timeDelta;
      } else {
        timeDelta = duration;
        duration = 0;
      }
      price += rate * timeDelta;

      LOG_DEBUG("{} Δt:{} price:{}, duration:{}", toString(ticker), timeDelta, price, duration);

      if (duration == 0) {
        randomize(timeStamp);
      } else {
        lastUpdate = timeStamp;
      }
      return data.getPrice() != getPrice();
    }

    Price getPrice() const { return static_cast<Price>(std::round(price)); }

    const Ticker ticker;
    const TickerData &data;

    double price{0};
    double rate{0};
    const double drift{0};
    size_t duration{0};
    Timestamp lastUpdate{0};
  };

public:
  PriceFeed(Bus &bus, CRef<MarketData> marketData)
      : bus_{bus}, priceUpdateTimer_{bus_.systemCtx()},
        updateInterval_{ServerConfig::cfg.priceFeedRate} {
    fluctuations_.reserve(marketData.size());
    for (auto &value : marketData) {
      fluctuations_.push_back(Fluctuation(value.first, *value.second));
    }
    bus_.systemBus.subscribe<ServerCommand>(ServerCommand::PriceFeedStart, [this] { start(); });
    bus_.systemBus.subscribe<ServerCommand>(ServerCommand::PriceFeedStop, [this] { stop(); });
  }

  void start() {
    LOG_INFO_SYSTEM("Start broadcasting price changes");
    updating_ = true;
    schedulePriceUpdate();
  }

  void stop() {
    LOG_INFO_SYSTEM("Stop broadcasting price changes");
    priceUpdateTimer_.cancel();
    updating_ = false;
  }

private:
  void schedulePriceUpdate() {
    if (!updating_) {
      return;
    }
    priceUpdateTimer_.expires_after(updateInterval_);
    priceUpdateTimer_.async_wait([this](BoostErrorCode ec) {
      if (ec) {
        return;
      }
      updatePrices();
      schedulePriceUpdate();
    });
  }

  void updatePrices() {
    const auto timeStamp = utils::getTimestamp();
    for (auto &item : fluctuations_) {
      if (item.update(timeStamp)) {
        LOG_DEBUG("Price changed for {}: {}=>{}", utils::toString(item.ticker),
                  item.data.getPrice(), item.getPrice());
        const auto newPrice = item.getPrice();
        item.data.setPrice(newPrice);
        bus_.marketBus.post(TickerPrice{item.ticker, newPrice});
      }
    }
  }

private:
  Bus &bus_;

  std::vector<Fluctuation> fluctuations_;

  SteadyTimer priceUpdateTimer_;
  const Microseconds updateInterval_;

  std::atomic_bool updating_{false};
};

} // namespace hft::server

#endif // HFT_SERVER_PRICEFEED_HPP
