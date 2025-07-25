/**
 * @author Vladimir Pavliv
 * @date 2025-04-06
 */

#ifndef HFT_SERVER_CLIENTENGINE_HPP
#define HFT_SERVER_CLIENTENGINE_HPP

#include <boost/unordered/unordered_flat_map.hpp>

#include "adapters/postgres/postgres_adapter.hpp"
#include "client_ticker_data.hpp"
#include "client_types.hpp"
#include "config/client_config.hpp"
#include "ctx_runner.hpp"
#include "metadata_types.hpp"
#include "rtt_tracker.hpp"
#include "types.hpp"
#include "utils/market_utils.hpp"
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
  using Tracker = RttTracker<50>;
  using TickersData = boost::unordered_flat_map<Ticker, UPtr<TickerData>, TickerHash>;

  explicit TradeEngine(Bus &bus)
      : bus_{bus}, worker_{makeWorker()}, tradeTimer_{worker_.ioCtx}, statsTimer_{bus_.systemCtx()},
        tradeRate_{ClientConfig::cfg.tradeRate}, monitorRate_{ClientConfig::cfg.monitorRate} {
    // Market connectors
    bus_.marketBus.setHandler<OrderStatus>(
        [this](CRef<OrderStatus> status) { onOrderStatus(status); });
    bus_.marketBus.setHandler<TickerPrice>(
        [this](CRef<TickerPrice> price) { onTickerPrice(price); });

    bus_.systemBus.subscribe<ClientEvent>([this](CRef<ClientEvent> event) {
      switch (event) {
      case ClientEvent::Connected:
        LOG_INFO_SYSTEM("Ready to start trade");
        operational_ = true;
        break;
      case ClientEvent::ConnectionFailed:
      case ClientEvent::Disconnected:
      case ClientEvent::InternalError:
        operational_ = false;
        tradeStop();
        break;
      default:
        break;
      }
    });

    bus_.systemBus.subscribe(ClientCommand::TradeStart, [this] { tradeStart(); });
    bus_.systemBus.subscribe(ClientCommand::TradeStop, [this] { tradeStop(); });
    bus_.systemBus.subscribe(ClientCommand::TradeSpeedUp, [this] {
      if (tradeRate_ > Microseconds(1)) {
        tradeRate_ /= 2;
        LOG_INFO_SYSTEM("Trade rate: {}", tradeRate_.count());
      }
    });
    bus_.systemBus.subscribe(ClientCommand::TradeSpeedDown, [this] {
      tradeRate_ *= 2;
      LOG_INFO_SYSTEM("Trade rate: {}", tradeRate_.count());
    });
  }

  void start() {
    LOG_DEBUG("Starting trade engine");
    startWorkers();
    loadMarketData();
  }

  void stop() {
    LOG_DEBUG("Stoping trade engine");
    worker_.stop();
  }

private:
  void startWorkers() {
    // Design is not yet clear here so a single worker for now
    LOG_INFO_SYSTEM("Starting trade worker");
    worker_.run();
  }

  void tradeStart() {
    LOG_INFO_SYSTEM("Trade start");
    if (!operational_) {
      LOG_ERROR_SYSTEM("Not connected to the server");
      return;
    }
    trading_ = true;
    scheduleTradeTimer();
    scheduleStatsTimer();
  }

  void tradeStop() {
    tradeTimer_.cancel();
    if (trading_) {
      LOG_INFO_SYSTEM("Trade stop");
      trading_ = false;
    }
  }

  void scheduleTradeTimer() {
    if (!operational_) {
      LOG_ERROR_SYSTEM("Not connected to the server");
      return;
    }
    if (!trading_) {
      return;
    }
    tradeTimer_.expires_after(tradeRate_);
    tradeTimer_.async_wait([this](BoostErrorCode ec) {
      if (ec) {
        LOG_ERROR_SYSTEM("{}", ec.message());
        return;
      }
      tradeSomething();
      scheduleTradeTimer();
    });
  }

  void loadMarketData() {
    LOG_DEBUG("Loading data");
    const auto result = dbAdapter_.readTickers();
    if (!result) {
      LOG_ERROR("Failed to load ticker data");
      throw std::runtime_error(utils::toString(result.error()));
    }
    const auto &prices = result.value();
    tickersData_.reserve(prices.size());
    for (auto &price : prices) {
      tickersData_[price.ticker] = std::make_unique<TickerData>(price.price);
    }
    LOG_DEBUG("Data loaded for {} tickers", tickersData_.size());
  }

  void tradeSomething() {
    using namespace utils;
    static auto cursor = tickersData_.begin();
    if (cursor == tickersData_.end()) {
      cursor = tickersData_.begin();
    }
    auto &p = *cursor++;
    const auto newPrice = fluctuateThePrice(p.second->getPrice());
    const auto action = RNG::generate<uint8_t>(0, 1) == 0 ? OrderAction::Buy : OrderAction::Sell;
    const auto quantity = RNG::generate<Quantity>(0, 100);
    const auto id = getTimestamp(); // TODO(self)
    Order order{id, id, p.first, quantity, newPrice, action};
    LOG_TRACE("Placing order {}", utils::toString(order));
    bus_.marketBus.post(order);
  }

  void onOrderStatus(CRef<OrderStatus> s) {
    LOG_DEBUG("{}", utils::toString(s));
    // Track orders in TickerData
    Tracker::logRtt(s.orderId);
    bus_.systemBus.post(OrderTimestamp{s.orderId, s.orderId, s.fulfilled, utils::getTimestamp()});
  }

  void onTickerPrice(CRef<TickerPrice> price) {
    const auto dataIt = tickersData_.find(price.ticker);
    if (dataIt == tickersData_.end()) {
      LOG_ERROR("Ticker {} not found", utils::toString(price.ticker));
      return;
    }
    const Price oldPrice = dataIt->second->getPrice();
    LOG_DEBUG("Price change {}: {} => {}", utils::toString(price.ticker), oldPrice, price.price);
    dataIt->second->setPrice(price.price);
  }

  void scheduleStatsTimer() {
    if (!trading_ || !operational_ || monitorRate_.count() == 0) {
      return;
    }
    statsTimer_.expires_after(monitorRate_);
    statsTimer_.async_wait([this](BoostErrorCode ec) {
      if (ec) {
        LOG_ERROR_SYSTEM("{}", ec.message());
        return;
      }
      LOG_INFO_SYSTEM("Rtt: {}", Tracker::getRttStats());
      scheduleStatsTimer();
    });
  }

  CtxRunner makeWorker() {
    if (ClientConfig::cfg.coresApp.empty()) {
      return CtxRunner(0, false);
    } else {
      return CtxRunner(0, true, ClientConfig::cfg.coresApp[0]);
    }
  }

private:
  Bus &bus_;

  PostgresAdapter dbAdapter_;
  TickersData tickersData_;

  // For now a single worker
  CtxRunner worker_;

  SteadyTimer tradeTimer_;
  SteadyTimer statsTimer_;

  Microseconds tradeRate_;
  Seconds monitorRate_;
  std::atomic_bool operational_{false};
  std::atomic_bool trading_{false};
};
} // namespace hft::client

#endif // HFT_SERVER_CLIENTENGINE_HPP