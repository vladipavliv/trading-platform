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
#include "id/slot_id_pool.hpp"
#include "market_data.hpp"
#include "primitive_types.hpp"
#include "traits.hpp"
#include "utils/ctx_runner.hpp"
#include "utils/huge_array.hpp"
#include "utils/market_utils.hpp"
#include "utils/rng.hpp"
#include "utils/rtt_tracker.hpp"
#include "utils/telemetry_utils.hpp"
#include "utils/time_utils.hpp"

namespace hft::client {

/**
 * @brief Performs the trading
 * @details For now generates random orders, tracks rtt and price updates
 * Later on maybe will add proper algorithms
 */
class TradeEngine {
  static constexpr size_t ORDER_LIMIT = 16777216;
  // static constexpr size_t ORDER_LIMIT = 524288;

  struct ClientOrder {
    Order order;
    Timestamp created;
    SlotIdPool<>::IdType id;
  };

public:
  explicit TradeEngine(ClientBus &bus) : bus_{bus}, marketData_{loadMarketData()} {
    bus_.subscribe<OrderStatus>(
        CRefHandler<OrderStatus>::template bind<TradeEngine, &TradeEngine::post>(this));
    bus_.subscribe<TickerPrice>(
        CRefHandler<TickerPrice>::template bind<TradeEngine, &TradeEngine::post>(this));
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
    LOG_INFO_SYSTEM("Stopping trade engine");
    running_.store(false, std::memory_order_release);
    if (worker_.joinable()) {
      worker_.join();
    }
  }

  void tradeStart() {
    if (trading_) {
      return;
    }
    LOG_INFO_SYSTEM("Trade start");
    trading_ = true;
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
        if (!ClientConfig::cfg().coresApp.empty()) {
          utils::pinThreadToCore(ClientConfig::cfg().coresApp[0]);
        }
        tradeLoop();
      } catch (const std::exception &ex) {
        LOG_ERROR_SYSTEM("Exception in trade engine loop {}", ex.what());
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
    uint32_t warmupCount = Config::get<uint64_t>("rates.warmup");
    uint32_t counter = 0;
    while (running_.load(std::memory_order_acquire)) {
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
      const auto quantity = RNG::generate<Quantity>(1, 100);

      // auto id = idPool_.acquire();
      auto id = ++counter;
      const auto now = getCycles();
      Order order{id, now, p.first, quantity, newPrice, action};

      // orders_[id.index()] = {order, now, id};

      placed_.fetch_add(1, std::memory_order_relaxed);
      LOG_TRACE("Placing order {}", toString(order));
      bus_.marketBus.post(order);

      for (int i = 0; i < ClientConfig::cfg().tradeRate; ++i) {
        asm volatile("pause" ::: "memory");
      }
    }
  }

  void post(CRef<OrderStatus> s) {
    using namespace utils;
    LOG_DEBUG("{}", toString(s));
    switch (s.state) {
    case OrderState::Rejected:
      LOG_ERROR_SYSTEM("{}", toString(s));
      LOG_ERROR_SYSTEM("Order was rejected, stopping");
      tradeStop();
      break;
    default:
      const auto now = getCycles();
      LOG_DEBUG("Post Order telemetry");

      // auto &o = orders_[s.orderId];
      bus_.post(createOrderLatencyMsg(Source::Client, 0, s.orderId, s.timeStamp, 0, now,
                                      placed_.load(std::memory_order_relaxed),
                                      fulfilled_.load(std::memory_order_relaxed)));

      switch (s.state) {
      case OrderState::Full:
        fulfilled_.fetch_add(1, std::memory_order_relaxed);
        break;
      case OrderState::Rejected:
        stop();
      default:
        break;
      }

      break;
    }
  }

  void post(CRef<TickerPrice> price) {
    const auto dataIt = marketData_.find(price.ticker);
    if (dataIt == marketData_.end()) {
      LOG_ERROR("Ticker {} not found", toString(price.ticker));
      return;
    }
    const Price oldPrice = dataIt->second.getPrice();
    LOG_DEBUG("Price change {}: {} => {}", toString(price.ticker), oldPrice, price.price);
    dataIt->second.setPrice(price.price);
  }

private:
  DbAdapter dbAdapter_;
  const MarketData marketData_;

  ClientBus &bus_;
  std::jthread worker_;

  // SlotIdPool<> idPool_;
  // HugeArray<ClientOrder, ORDER_LIMIT> orders_;

  alignas(64) AtomicUInt64 placed_{0};
  alignas(64) AtomicUInt64 fulfilled_{0};

  alignas(64) AtomicBool running_{false};
  alignas(64) AtomicBool trading_{false};
};
} // namespace hft::client

#endif // HFT_SERVER_CLIENTENGINE_HPP
