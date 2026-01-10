/**
 * @author Vladimir Pavliv
 * @date 2025-04-06
 */

#ifndef HFT_SERVER_CLIENTENGINE_HPP
#define HFT_SERVER_CLIENTENGINE_HPP

#include <boost/unordered/unordered_flat_map.hpp>

#include "bus/bus_hub.hpp"
#include "commands/command.hpp"
#include "config/client_config.hpp"
#include "config/config.hpp"
#include "ctx_runner.hpp"
#include "market_data.hpp"
#include "metadata_types.hpp"
#include "primitive_types.hpp"
#include "rtt_tracker.hpp"
#include "traits.hpp"
#include "utils/rng.hpp"
#include "utils/test_utils.hpp"
#include "utils/time_utils.hpp"

namespace hft::client {

/**
 * @brief Performs the trading
 * @details For now generates random orders, tracks rtt and price updates
 * Later on maybe will add proper algorithms
 */
class TradeEngine {
public:
  using Tracker = RttTracker<1000, 10000>;

  explicit TradeEngine(ClientBus &bus)
      : bus_{bus}, marketData_{loadMarketData()}, statsTimer_{bus_.systemIoCtx()} {
    bus_.subscribe<OrderStatus>([this](CRef<OrderStatus> status) { onOrderStatus(status); });
    bus_.subscribe<TickerPrice>([this](CRef<TickerPrice> price) { onTickerPrice(price); });

    bus_.systemBus.subscribe<Command>(Command::Telemetry_Start, [this] {
      LOG_INFO_SYSTEM("Start telemetry stream");
      telemetry_ = true;
    });
    bus_.systemBus.subscribe(Command::Telemetry_Stop, [this] {
      LOG_INFO_SYSTEM("Stop telemetry stream");
      telemetry_ = false;
    });
  }

  void start() {
    if (running_) {
      return;
    }
    LOG_DEBUG("Starting trade engine");
    running_ = true;
    startWorkers();
  }

  void stop() {
    LOG_DEBUG("Stopping trade engine");
    running_ = false;
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
    worker_ = std::jthread{[this]() {
      try {
        utils::setThreadRealTime();
        if (!ClientConfig::cfg.coresApp.empty()) {
          utils::pinThreadToCore(ClientConfig::cfg.coresApp[0]);
        }
        tradeLoop();
      } catch (const std::exception &ex) {
        LOG_ERROR("Exception in trade engine loop {}", ex.what());
        bus_.post(InternalError{StatusCode::Error, ex.what()});
      }
    }};
  }

  auto loadMarketData() -> MarketData {
    LOG_DEBUG("Loading data");
    const auto result = dbAdapter_.readTickers();
    if (!result || result.value().empty()) {
      LOG_ERROR("Failed to load market data");
      throw std::runtime_error(toString(result.error()));
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
    uint64_t warmupCount = Config::get<uint64_t>("rates.warmup");
    uint64_t counter = 0;
    while (running_) {
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
      const auto newPrice = utils::fluctuateThePrice(p.second.getPrice());
      const auto action = RNG::generate<uint8_t>(0, 1) == 0 ? OrderAction::Buy : OrderAction::Sell;
      const auto quantity = RNG::generate<Quantity>(0, 100);
      const auto id = getCycles();
      Order order{id, id, p.first, quantity, newPrice, action};

      if (counter < warmupCount) {
        ++counter;
        order.action = OrderAction::Dummy;
      }
      LOG_TRACE("Placing order {}", toString(order));
      bus_.marketBus.post(order);

      for (int i = 0; i < ClientConfig::cfg.tradeRate; ++i) {
        asm volatile("pause" ::: "memory");
      }
    }
  }

  void onOrderStatus(CRef<OrderStatus> s) {
    using namespace utils;
    LOG_DEBUG("{}", toString(s));
    switch (s.state) {
    case OrderState::Rejected:
      LOG_ERROR_SYSTEM("Order {} was rejected", s.orderId);
      tradeStop();
      break;
    default:
      const auto rtt = (getCycles() - s.orderId) * ClientConfig::cfg.nsPerCycle;
      Tracker::logRtt(rtt);
#ifdef TELEMETRY_ENABLED
      if (telemetry_) {
        bus_.post(OrderTimestamp{s.orderId, s.orderId, s.timeStamp, now});
      }
#endif
      break;
    }
  }

  void onTickerPrice(CRef<TickerPrice> price) {
    const auto dataIt = marketData_.find(price.ticker);
    if (dataIt == marketData_.end()) {
      LOG_ERROR("Ticker {} not found", toString(price.ticker));
      return;
    }
    const Price oldPrice = dataIt->second.getPrice();
    LOG_DEBUG("Price change {}: {} => {}", toString(price.ticker), oldPrice, price.price);
    dataIt->second.setPrice(price.price);
  }

  void scheduleStatsTimer() {
    if (!trading_) {
      return;
    }
    statsTimer_.expires_after(Milliseconds(ClientConfig::cfg.monitorRate));
    statsTimer_.async_wait([this](BoostErrorCode code) {
      if (code) {
        if (code != ERR_ABORTED) {
          LOG_ERROR_SYSTEM("{}", code.message());
        }
        return;
      }
      LOG_INFO_SYSTEM("Rtt: {}", Tracker::getStatsString());
      Tracker::reset();
      scheduleStatsTimer();
    });
  }

private:
  DbAdapter dbAdapter_;
  const MarketData marketData_;

  ClientBus &bus_;
  std::jthread worker_;

  SteadyTimer statsTimer_;

  std::atomic_bool running_{false};
  std::atomic_bool trading_{false};
  std::atomic_bool telemetry_{false};
};
} // namespace hft::client

#endif // HFT_SERVER_CLIENTENGINE_HPP
