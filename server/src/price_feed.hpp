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
    static constexpr size_t MIN_DURATION_US = // min period of 1s
        1000000;
    static constexpr size_t MAX_DURATION_US = // max period of 60s
        60000000;
    static constexpr double MIN_FLUCTUATION_RATE_US = // 1% per day in us
        1.0 / 86400.0 / 1000.0 / 1000.0;
    static constexpr double MAX_FLUCTUATION_RATE_US = // 100% per day in us
        100.0 / 86400.0 / 1000.0 / 1000.0;

    Fluctuation(CRef<Ticker> ticker, CRef<TickerData> data) : ticker{ticker}, data{data} {
      randomize();
    };

    bool update() {
      using namespace utils;
      if (durationUs == 0) {
        randomize();
        return false;
      }
      const auto timeStamp = getTimestamp();
      const auto timeDelta = timeStamp - lastUpdate;
      const auto priceDelta = speed * timeDelta;
      if (direction) {
        accumulatedPrice += priceDelta;
      } else {
        accumulatedPrice -= priceDelta;
      }
      if (durationUs > timeDelta) {
        durationUs -= timeDelta;
      } else {
        durationUs = 0;
      }
      LOG_DEBUG("{} Time delta:{} Price delta:{} accumulatedPrice:{}, durationUs:{}",
                utils::toString(ticker), timeDelta, priceDelta, accumulatedPrice, durationUs);
      lastUpdate = timeStamp;
      return data.getPrice() != currentPrice();
    }

    void randomize() {
      using namespace utils;
      direction = RNG::generate<uint8_t>(0, 1) == 0;
      accumulatedPrice = static_cast<double>(data.getPrice());
      speed = accumulatedPrice *
              RNG::generate<double>(MIN_FLUCTUATION_RATE_US, MAX_FLUCTUATION_RATE_US);
      durationUs = RNG::generate<size_t>(MIN_DURATION_US, MAX_DURATION_US);
      lastUpdate = getTimestamp();
      LOG_DEBUG("{} price:{} step:{} duration:{}", utils::toString(ticker), accumulatedPrice, speed,
                durationUs);
    }

    Price currentPrice() const { return static_cast<Price>(std::round(accumulatedPrice)); }

    const Ticker ticker;
    const TickerData &data;

    bool direction{true};
    double accumulatedPrice{0};
    double speed{0};
    size_t durationUs{0};
    Timestamp lastUpdate{0};
  };

public:
  PriceFeed(Bus &bus, CRef<MarketData> marketData)
      : bus_{bus}, timer_{bus_.systemCtx()}, rate_{ServerConfig::cfg.priceFeedRate} {
    data_.reserve(marketData.size());
    for (auto &value : marketData) {
      data_.push_back(Fluctuation(value.first, *value.second));
    }
    bus_.systemBus.subscribe<ServerCommand>(ServerCommand::PriceFeedStart, [this] { start(); });
    bus_.systemBus.subscribe<ServerCommand>(ServerCommand::PriceFeedStop, [this] { stop(); });
  }

  void start() {
    LOG_INFO_SYSTEM("Start broadcasting price changes");
    broadcasting_ = true;
    schedulePriceUpdate();
  }

  void stop() {
    LOG_INFO_SYSTEM("Stop broadcasting price changes");
    timer_.cancel();
    broadcasting_ = false;
  }

private:
  void schedulePriceUpdate() {
    if (!broadcasting_) {
      return;
    }
    timer_.expires_after(rate_);
    timer_.async_wait([this](BoostErrorCode ec) {
      if (ec) {
        return;
      }
      updatePrices();
      schedulePriceUpdate();
    });
  }

  void updatePrices() {
    for (auto &item : data_) {
      if (item.update()) {
        LOG_DEBUG("Price changed for {}: {}=>{}", utils::toString(item.ticker),
                  item.data.getPrice(), item.currentPrice());
        const auto newPrice = item.currentPrice();
        item.data.setPrice(newPrice);
        bus_.marketBus.post(TickerPrice{item.ticker, newPrice});
      }
    }
  }

private:
  Bus &bus_;

  std::vector<Fluctuation> data_;

  SteadyTimer timer_;
  const Microseconds rate_;

  std::atomic_bool broadcasting_{false};
};

} // namespace hft::server

#endif // HFT_SERVER_PRICEFEED_HPP
