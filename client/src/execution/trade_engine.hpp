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
#include "containers/huge_array.hpp"
#include "containers/sequenced_spsc.hpp"
#include "id/slot_id_pool.hpp"
#include "market_data.hpp"
#include "primitive_types.hpp"
#include "runner/ctx_runner.hpp"
#include "traits.hpp"
#include "utils/market_utils.hpp"
#include "utils/rng.hpp"
#include "utils/rtt_tracker.hpp"
#include "utils/string_utils.hpp"
#include "utils/telemetry_utils.hpp"
#include "utils/thread_utils.hpp"
#include "utils/time_utils.hpp"

namespace hft::client {

/**
 * @brief Generates random orders for each ticker, tracks the statuses
 * randomly cancels some of the orders after they have been accepted by the server
 * streams telemetry to the monitor
 */
class TradeEngine {
  using SelfT = TradeEngine;
  using SystemOId = SlotIdPool<>::IdType;
  /**
   * @brief Tracks the generated order, and the server-side id for modifications
   */
  struct ClientOrder {
    Order order;
    Timestamp created;
    SystemOId sysOId;

    bool isValid() const noexcept { return created != 0 && sysOId.isValid(); }
  };

public:
  explicit TradeEngine(Context &ctx)
      : ctx_{ctx}, dbAdapter_{ctx_.config.data}, marketData_{loadMarketData()},
        timer_{ctx_.bus.systemIoCtx()} {
    ctx_.bus.subscribe(CRefHandler<OrderStatus>::bind<SelfT, &SelfT::post>(this));
    ctx_.bus.subscribe(CRefHandler<TickerPrice>::bind<SelfT, &SelfT::post>(this));
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
    data.reserve(prices.size());
    for (auto &price : prices) {
      data.emplace(price.ticker, price.price);
    }
    LOG_DEBUG("Data loaded for {} tickers", data.size());
    return data;
  }

  void tradeLoop() {
    using namespace utils;
    uint32_t warmupCount = ctx_.config.data.get<uint64_t>("rates.warmup");
    uint32_t counter = 0;
    while (!ctx_.stopToken.stop_requested()) {
      if (!trading_) {
        std::this_thread::yield();
        continue;
      }

      if (!sendCancel() && !sendNew()) {
        break;
      }

      for (int i = 0; i < ctx_.config.tradeRate; ++i) {
        asm volatile("pause" ::: "memory");
        if (ctx_.stopToken.stop_requested()) {
          return;
        }
      }
    }
  }

  bool sendNew() {
    using namespace utils;
    static auto cursor = marketData_.begin();
    if (cursor == marketData_.end()) {
      cursor = marketData_.begin();
    }
    auto &p = *cursor++;
    const auto newPrice = fluctuateThePrice(p.second.getPrice());
    const auto action = RNG::generate<uint8_t>(0, 1) == 0 ? OrderAction::Buy : OrderAction::Sell;
    const auto quantity = RNG::generate<Quantity>(1, 100);

    auto id = idPool_.acquire();
    if (!id) {
      LOG_ERROR_SYSTEM("Failed to acquire fresh id, stopping");
      stop();
      return false;
    }
    const auto now = getCycles();
    Order order{id.raw(), p.first, quantity, newPrice, action};

    orders_[id.index()] = {order, now, id};

    placed_.fetch_add(1, std::memory_order_relaxed);
    LOG_DEBUG("Placing order {}", toString(order));
    ctx_.bus.marketBus.post(order);
    return true;
  }

  bool sendCancel() {
    uint32_t idx;
    if (toCancel_.read(idx)) {
      auto &r = orders_[idx];
      auto &o = r.order;
      r.created = utils::getCycles();
      Order toCancelO{r.sysOId.raw(), o.ticker, o.quantity, o.price, OrderAction::Cancel};
      LOG_DEBUG("Posting cancel {}", toString(toCancelO));
      ctx_.bus.marketBus.post(toCancelO);
      return true;
    }
    return false;
  }

  void post(CRef<OrderStatus> s) {
    LOG_DEBUG("{}", toString(s));
    using namespace utils;
    if (ctx_.stopToken.stop_requested()) {
      return;
    }
    LOG_DEBUG("{}", toString(s));
    auto soid = SystemOId{s.orderId};
    if (!soid.isValid()) {
      LOG_ERROR_SYSTEM("Invalid external order id {} {}", s.orderId, toString(s));
      stop();
      return;
    }
    auto &r = orders_[soid.index()];
    if (!r.isValid()) {
      LOG_ERROR_SYSTEM("Invalid record at {}", soid.index());
      stop();
      return;
    }

    auto &o = r.order;
    r.sysOId = SystemOId{s.systemOrderId};
    const auto cycl = getCycles();
    const auto plcd = placed_.load(std::memory_order_relaxed);
    const auto fulf = fulfilled_.load(std::memory_order_relaxed);
    ctx_.bus.post(
        createOrderLatencyMsg(Source::Client, 0, s.orderId, r.created, 0, cycl, plcd, fulf));

    switch (s.state) {
    case OrderState::Accepted: {
      if (RNG::generate<uint32_t>(0, 1) == 1) {
        toCancel_.write(soid.index());
      }
      break;
    }
    case OrderState::Full: {
      fulfilled_.fetch_add(1, std::memory_order_relaxed);
      idPool_.release(soid);
      break;
    }
    case OrderState::Cancelled: {
      cancelled_.fetch_add(1, std::memory_order_relaxed);
      idPool_.release(soid);
      break;
    }
    case OrderState::Rejected:
      LOG_WARN("Order rejected {}", toString(s));
      idPool_.release(soid);
      break;
    default:
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

  void scheduleStats() {
    using namespace utils;
    timer_.expires_after(Seconds(1));
    timer_.async_wait([this](BoostErrorCode code) {
      if (code || ctx_.stopToken.stop_requested()) {
        return;
      }
      static size_t lastCounter = 0;
      auto placed = placed_.load(std::memory_order_relaxed);
      auto fulfil = fulfilled_.load(std::memory_order_relaxed);
      auto cancel = cancelled_.load(std::memory_order_relaxed);
      size_t counter = placed + fulfil + cancel;
      if (lastCounter != counter) {
        LOG_INFO_SYSTEM("Placed:{} Closed:{} Cancelled:{}", formatCompact(placed),
                        formatCompact(fulfil), formatCompact(cancel));
      }
      lastCounter = counter;
      scheduleStats();
    });
  }

private:
  Context &ctx_;

  DbAdapter dbAdapter_;
  const MarketData marketData_;

  ALIGN_CL SlotIdPool<> idPool_;
  ALIGN_CL HugeArray<ClientOrder, MAX_SYSTEM_ORDERS> orders_;

  ALIGN_CL AtomicUInt64 placed_{0};
  ALIGN_CL AtomicUInt64 fulfilled_{0};
  ALIGN_CL AtomicUInt64 cancelled_{0};

  ALIGN_CL AtomicBool started_{false};
  ALIGN_CL AtomicBool trading_{false};

  ALIGN_CL SequencedSPSC<1024> toCancel_;
  ALIGN_CL SteadyTimer timer_;

  std::jthread worker_;
};
} // namespace hft::client

#endif // HFT_SERVER_CLIENTENGINE_HPP
