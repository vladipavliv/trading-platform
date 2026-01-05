/**
 * @author Vladimir Pavliv
 * @date 2025-04-06
 */

#ifndef HFT_SERVER_CLIENTENGINE_HPP
#define HFT_SERVER_CLIENTENGINE_HPP

#include <boost/unordered/unordered_flat_map.hpp>

#include "adapters/adapters.hpp"
#include "client_market_data.hpp"
#include "client_types.hpp"
#include "config/client_config.hpp"
#include "ctx_runner.hpp"
#include "metadata_types.hpp"
#include "rtt_tracker.hpp"
#include "types.hpp"
#include "utils/rng.hpp"
#include "utils/utils.hpp"

namespace hft::client {

/**
 * @brief Performs the trading
 * @details For now generates random orders, tracks rtt and price updates
 * Later on maybe will add proper algorithms
 */
class TradeEngine {
public:
  using Tracker = RttTracker<1000>;

  explicit TradeEngine(ClientBus &bus)
      : bus_{bus}, marketData_{loadMarketData()}, statsTimer_{bus_.systemIoCtx()} {
    bus_.subscribe<OrderStatus>([this](CRef<OrderStatus> status) { onOrderStatus(status); });
    bus_.subscribe<TickerPrice>([this](CRef<TickerPrice> price) { onTickerPrice(price); });

    bus_.systemBus.subscribe(ClientCommand::Telemetry_Start, [this] {
      LOG_INFO_SYSTEM("Start telemetry stream");
      telemetry_ = true;
    });
    bus_.systemBus.subscribe(ClientCommand::Telemetry_Stop, [this] {
      LOG_INFO_SYSTEM("Stop telemetry stream");
      telemetry_ = false;
    });
  }

  void start() {
    if (started_) {
      return;
    }
    LOG_DEBUG("Starting trade engine");
    started_ = true;
    startWorkers();
  }

  void stop() {
    LOG_DEBUG("Stopping trade engine");
    started_ = false;
  }

  void tradeStart() {
    if (trading_) {
      return;
    }
    LOG_INFO_SYSTEM("Trade start");
    trading_ = true;
    scheduleStatsTimer();
  }

  void tradeStop() {
    if (!trading_) {
      return;
    }
    LOG_INFO_SYSTEM("Trade stop");
    trading_ = false;
  }

private:
  void startWorkers() {
    LOG_INFO_SYSTEM("Starting trade worker");
    worker_ = Thread{[this]() {
      utils::setTheadRealTime();
      if (!ClientConfig::cfg.coresApp.empty()) {
        utils::pinThreadToCore(ClientConfig::cfg.coresApp[0]);
      }
      tradeLoop();
    }};
  }

  auto loadMarketData() -> MarketData {
    LOG_DEBUG("Loading data");
    const auto result = dbAdapter_.readTickers();
    if (!result || result.value().empty()) {
      LOG_ERROR("Failed to load market data");
      throw std::runtime_error(utils::toString(result.error()));
    }
    MarketData data;
    const auto &prices = result.value();
    data.reserve(prices.size());
    for (auto &price : prices) {
      data.emplace(price.ticker, price.price);
    }
    LOG_DEBUG("Data loaded for {} tickers", data.size());
    return data;
  }

  void tradeLoop() {
    while (started_) {
      if (!trading_) {
        asm volatile("pause" ::: "memory");
        continue;
      }
      using namespace utils;
      static auto cursor = marketData_.begin();
      if (cursor == marketData_.end()) {
        cursor = marketData_.begin();
      }
      auto &p = *cursor++;
      const auto newPrice = fluctuateThePrice(p.second.getPrice());
      const auto action = RNG::generate<uint8_t>(0, 1) == 0 ? OrderAction::Buy : OrderAction::Sell;
      const auto quantity = RNG::generate<Quantity>(0, 100);
      const auto id = getTimestampNs(); // TODO(self)
      Order order{id, id, p.first, quantity, newPrice, action};
      LOG_TRACE("Placing order {}", utils::toString(order));
      bus_.marketBus.post(order);

      for (int i = 0; i < 10; ++i) {
        std::this_thread::yield();
      }
      // std::this_thread::yield();
      std::this_thread::sleep_for(std::chrono::microseconds(ClientConfig::cfg.tradeRate));
    }
  }

  void onOrderStatus(CRef<OrderStatus> s) {
    using namespace utils;
    LOG_DEBUG("{}", utils::toString(s));
    switch (s.state) {
    case OrderState::Rejected:
      LOG_ERROR_SYSTEM("Order {} was rejected", s.orderId);
      tradeStop();
      break;
    default:
      Tracker::logRtt(s.orderId);
#ifdef TELEMETRY_ENABLED
      if (telemetry_) {
        bus_.post(OrderTimestamp{s.orderId, s.orderId, s.timeStamp, getTimestampNs()});
      }
#endif
      break;
    }
  }

  void onTickerPrice(CRef<TickerPrice> price) {
    const auto dataIt = marketData_.find(price.ticker);
    if (dataIt == marketData_.end()) {
      LOG_ERROR("Ticker {} not found", utils::toString(price.ticker));
      return;
    }
    const Price oldPrice = dataIt->second.getPrice();
    LOG_DEBUG("Price change {}: {} => {}", utils::toString(price.ticker), oldPrice, price.price);
    dataIt->second.setPrice(price.price);
  }

  void scheduleStatsTimer() {
    if (!trading_) {
      return;
    }
    statsTimer_.expires_after(ClientConfig::cfg.monitorRate);
    statsTimer_.async_wait([this](BoostErrorCode code) {
      if (code) {
        if (code != ASIO_ERR_ABORTED) {
          LOG_ERROR_SYSTEM("{}", code.message());
        }
        return;
      }
      LOG_INFO_SYSTEM("Rtt: {}", Tracker::getStatsString());
      scheduleStatsTimer();
    });
  }

private:
  adapters::DbAdapter dbAdapter_;
  const MarketData marketData_;

  ClientBus &bus_;
  Thread worker_;

  SteadyTimer statsTimer_;

  std::atomic_bool started_{false};
  std::atomic_bool trading_{false};
  std::atomic_bool telemetry_{false};
};
} // namespace hft::client

#endif // HFT_SERVER_CLIENTENGINE_HPP
