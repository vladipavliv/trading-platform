/**
 * @author Vladimir Pavliv
 * @date 2025-04-06
 */

#ifndef HFT_SERVER_CLIENTENGINE_HPP
#define HFT_SERVER_CLIENTENGINE_HPP

#include "bus/bus_hub.hpp"
#include "commands/command.hpp"
#include "config/client_config.hpp"
#include "config/config.hpp"
#include "events.hpp"
#include "market_data.hpp"
#include "order_registry.hpp"
#include "primitive_types.hpp"
#include "runner/ctx_runner.hpp"
#include "strategy/strategies.hpp"
#include "traits.hpp"
#include "utils/handler.hpp"
#include "utils/market_utils.hpp"
#include "utils/rng.hpp"
#include "utils/string_utils.hpp"
#include "utils/telemetry_utils.hpp"
#include "utils/thread_utils.hpp"
#include "utils/time_utils.hpp"

namespace hft::client {

/**
 * @brief Runs the strategies
 */
class TradeEngine {
  using SelfT = TradeEngine;

public:
  explicit TradeEngine(Context &ctx)
      : ctx_{ctx}, dbAdapter_{ctx_.config.data}, marketData_{loadMarketData()},
        strategies_{RandomStrategy{ctx_, registry_, marketData_},
                    TtlCancelStrategy{ctx_, registry_}},
        timer_{ctx_.bus.systemIoCtx()} {
    ctx_.bus.subscribe(CRefHandler<OrderStatus>::bind<SelfT, &SelfT::post>(this));
    ctx_.bus.subscribe(CRefHandler<MarkPrice>::bind<SelfT, &SelfT::post>(this));
  }

  void start() {
    if (started_) {
      LOG_ERROR_SYSTEM("Already started");
      return;
    }
    LOG_DEBUG("Starting trade engine");
    started_ = true;
    startWorkers();
    scheduleStats();
  }

  void stop() {
    LOG_INFO_SYSTEM("Stopping trade engine");
    utils::join(worker_);
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
        if (!ctx_.config.coresApp.empty()) {
          utils::pinThreadToCore(ctx_.config.coresApp[0]);
        }
        ctx_.bus.post(ComponentReady{Component::Engine});
        tradeLoop();
      } catch (const std::exception &ex) {
        LOG_ERROR_SYSTEM("Exception in trade engine loop {}", ex.what());
        ctx_.bus.post(InternalError{StatusCode::Error, ex.what()});
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

    if (prices.size() != TICKER_COUNT) {
      throw std::runtime_error(
          std::format("Invalid ticker count expected:{} got:{}", TICKER_COUNT, prices.size()));
    }
    uint32_t idx = 0;
    for (auto &price : prices) {
      data[getTickerIndex(price.ticker)].setPrice(price.price);
    }
    LOG_DEBUG_SYSTEM("Data loaded for {} tickers", data.size());
    return data;
  }

  void tradeLoop() {
    using namespace utils;
    while (!ctx_.stopToken.stop_requested()) {
      if (!trading_) {
        std::this_thread::yield();
        continue;
      }

      std::apply([this](auto &...s) { (s.execute(), ...); }, strategies_);
      speedBump();
    }
  }

  void speedBump() {
    for (int i = 0; i < ctx_.config.tradeRate; ++i) {
      asm volatile("pause" ::: "memory");
      if (ctx_.stopToken.stop_requested()) {
        return;
      }
    }
  }

  void post(CRef<OrderStatus> s) {
    LOG_DEBUG("{}", toString(s));
    using namespace utils;
    if (ctx_.stopToken.stop_requested()) {
      return;
    }
    const auto now = getCycles();
    const auto id = LocalOId{s.orderId};
    if (!id.isValid()) {
      LOG_ERROR_SYSTEM("Invalid external order id {} {}", s.orderId, toString(s));
      stop();
      return;
    }
    auto &r = registry_.records[id.index()];
    if (!r.isValid()) {
      LOG_ERROR_SYSTEM("Invalid record at {}", id.index());
      stop();
      return;
    }
    if (r.order.id != s.orderId) {
      LOG_ERROR_SYSTEM("Record order id mismatch at index {}", id.index());
      stop();
      return;
    }

    auto &o = r.order;
    const auto plcd = registry_.accepted.load(std::memory_order_relaxed);
    const auto fulf = registry_.fulfilled.load(std::memory_order_relaxed);
    ctx_.bus.post(
        createOrderLatencyMsg(Source::Client, 0, s.orderId, r.created, 0, now, plcd, fulf));

    switch (s.state) {
    case OrderState::Accepted: {
      r.sysOId = SystemOId{s.systemOrderId};
      r.setState(RecordState::Active);
      registry_.accepted.fetch_add(1, std::memory_order_relaxed);
      break;
    }
    case OrderState::Partial: {
      r.sysOId = SystemOId{s.systemOrderId};
      r.setState(RecordState::Active);
      registry_.accepted.fetch_add(1, std::memory_order_relaxed);
      break;
    }
    case OrderState::Full: {
      r.setState(RecordState::Closed);
      registry_.fulfilled.fetch_add(1, std::memory_order_relaxed);
      registry_.deallocate(id);
      break;
    }
    case OrderState::Cancelled: {
      r.setState(RecordState::Closed);
      registry_.cancelled.fetch_add(1, std::memory_order_relaxed);
      registry_.deallocate(id);
      break;
    }
    case OrderState::Rejected:
      LOG_WARN("Order rejected {}", toString(s));
      r.setState(RecordState::Closed);
      registry_.rejected.fetch_add(1, std::memory_order_relaxed);
      registry_.deallocate(id);
      break;
    default:
      break;
    }
  }

  void post(CRef<MarkPrice> price) {
    const auto idx = getTickerIndex(price.ticker);
    if (idx == -1) {
      LOG_ERROR("Ticker {} not found", toString(price.ticker));
      return;
    }
    auto &data = marketData_[idx];
    const Price oldPrice = data.getPrice();
    LOG_DEBUG("Price change {}: {} => {}", toString(price.ticker), oldPrice, price.price);
    data.setPrice(price.price);
  }

  void scheduleStats() {
    using namespace utils;
    timer_.expires_after(Seconds(1));
    timer_.async_wait([this](BoostErrorCode code) {
      if (code || ctx_.stopToken.stop_requested()) {
        return;
      }
      static size_t lastCounter = 0;
      const auto accepted = registry_.accepted.load(std::memory_order_relaxed);
      const auto fulfilled = registry_.fulfilled.load(std::memory_order_relaxed);
      const auto cancelled = registry_.cancelled.load(std::memory_order_relaxed);
      const auto rejected = registry_.rejected.load(std::memory_order_relaxed);
      size_t counter = accepted + fulfilled + cancelled + rejected;
      if (lastCounter != counter) {
        LOG_INFO_SYSTEM("Accepted:{} Fulfilled:{} Cancelled:{} Rejected:{}",
                        formatCompact(accepted), formatCompact(fulfilled), formatCompact(cancelled),
                        formatCompact(rejected));
      }
      lastCounter = counter;
      scheduleStats();
    });
  }

private:
  Context &ctx_;

  DbAdapter dbAdapter_;
  const MarketData marketData_;

  OrderRegistry registry_;
  Strategies strategies_;

  AtomicBool started_{false};
  AtomicBool trading_{false};

  SteadyTimer timer_;

  std::jthread worker_;
};
} // namespace hft::client

#endif // HFT_SERVER_CLIENTENGINE_HPP
