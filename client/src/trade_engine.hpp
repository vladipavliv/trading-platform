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
#include "metadata_types.hpp"
#include "rtt_tracker.hpp"
#include "types.hpp"
#include "utils/market_utils.hpp"
#include "utils/rng.hpp"
#include "utils/utils.hpp"
#include "worker.hpp"

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

    bus_.systemBus.subscribe(ClientEvent::ConnectedToTheServer, [this] {
      LOG_INFO_SYSTEM("Ready to start trade");
      operational_ = true;
    });
    bus_.systemBus.subscribe(ClientEvent::DisconnectedFromTheServer, [this] {
      operational_ = false;
      tradeStop();
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
    worker_.start();
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
    tradeTimer_.async_wait([this](CRef<BoostError> ec) {
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
      LOG_ERROR("Failed to load ticker data {}", result.error());
      throw std::runtime_error(result.error());
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
    const auto action = RNG::rng<uint8_t>(1) == 0 ? OrderAction::Buy : OrderAction::Sell;
    const auto quantity = RNG::rng<Quantity>(100);
    const auto id = getTimestamp(); // TODO(self)
    Order order{id, id, p.first, quantity, newPrice, action};
    LOG_DEBUG("Placing order {}", utils::toString(order));
    bus_.marketBus.post(order);
  }

  void onOrderStatus(CRef<OrderStatus> status) {
    LOG_DEBUG(utils::toString(status));
    // Track orders in TickerData
    Tracker::logRtt(status.orderId);
    bus_.systemBus.post(
        OrderTimestamp{status.orderId, status.orderId, status.fulfilled, utils::getTimestamp()});
  }

  void onTickerPrice(CRef<TickerPrice> price) {
    LOG_DEBUG(utils::toString(price));
    auto dataIt = tickersData_.find(price.ticker);
    if (dataIt == tickersData_.end()) {
      LOG_ERROR("Ticker {} not found", utils::toString(price.ticker));
      return;
    }
    dataIt->second->setPrice(price.price);
  }

  void scheduleStatsTimer() {
    if (!trading_ || !operational_ || monitorRate_.count() == 0) {
      return;
    }
    statsTimer_.expires_after(monitorRate_);
    statsTimer_.async_wait([this](CRef<BoostError> ec) {
      if (ec) {
        LOG_ERROR_SYSTEM("{}", ec.message());
        return;
      }
      LOG_INFO_SYSTEM("Rtt: {}", Tracker::getRttStats());
      scheduleStatsTimer();
    });
  }

  Worker makeWorker() {
    if (ClientConfig::cfg.coresApp.empty()) {
      return Worker(0, false);
    } else {
      return Worker(0, true, ClientConfig::cfg.coresApp[0]);
    }
  }

private:
  Bus &bus_;

  PostgresAdapter dbAdapter_;
  TickersData tickersData_;

  // For now a single worker
  Worker worker_;

  SteadyTimer tradeTimer_;
  SteadyTimer statsTimer_;

  Microseconds tradeRate_;
  Seconds monitorRate_;
  std::atomic_bool operational_{false};
  std::atomic_bool trading_{false};
};
} // namespace hft::client

#endif // HFT_SERVER_CLIENTENGINE_HPP