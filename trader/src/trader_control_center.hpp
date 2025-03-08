/**
 * @author Vladimir Pavliv
 * @date 2025-03-07
 */

#ifndef HFT_SERVER_TRADERCONTROLCENTER_HPP
#define HFT_SERVER_TRADERCONTROLCENTER_HPP

#include <atomic>
#include <vector>

#include "boost_types.hpp"
#include "config/config.hpp"
#include "console_manager.hpp"
#include "db/postgres_adapter.hpp"
#include "network_client.hpp"
#include "rtt_tracker.hpp"
#include "trader_bus.hpp"
#include "trader_command.hpp"
#include "trader_event.hpp"
#include "types.hpp"
#include "utils/market_utils.hpp"
#include "utils/rng.hpp"

namespace hft::trader {

class TraderControlCenter {
public:
  using UPtr = std::unique_ptr<TraderControlCenter>;
  using TraderConsoleManager = ConsoleManager<TraderCommand>;
  using Tracker = RttTracker<50, 200>; // TODO()

  TraderControlCenter()
      : networkClient_{bus_}, consoleManager_{bus_.systemBus}, tradeTimer_{bus_.ioCtx()},
        statsTimer_{bus_.ioCtx()}, tradeRate_{Config::cfg.tradeRate},
        monitorRate_{Config::cfg.monitorRate} {

    // Incoming market events
    bus_.marketBus.setHandler<OrderStatus>([this](Span<OrderStatus> events) {
      for (auto &status : events) {
        onOrderStatus(status);
      }
    });
    bus_.marketBus.setHandler<TickerPrice>([this](Span<TickerPrice> prices) {
      for (auto &price : prices) {
        onPriceUpdate(price);
      }
    });

    // System events
    bus_.systemBus.subscribe(TraderEvent::ConnectedToTheServer,
                             [this]() { Logger::monitorLogger->info("Connected to the server"); });
    bus_.systemBus.subscribe(TraderEvent::DisconnectedFromTheServer, [this]() {
      Logger::monitorLogger->error("Disconnected from the server");
      tradeStop();
    });

    // Console commands
    bus_.systemBus.subscribe(TraderCommand::Shutdown, [this]() { stop(); });
    bus_.systemBus.subscribe(TraderCommand::TradeStart, [this]() { tradeStart(); });
    bus_.systemBus.subscribe(TraderCommand::TradeStop, [this]() { tradeStop(); });
    bus_.systemBus.subscribe(TraderCommand::TradeSpeedUp, [this]() {
      if (tradeRate_ > Microseconds(1)) {
        tradeRate_ /= 2;
        Logger::monitorLogger->info(std::format("Trade rate: {}", tradeRate_));
      }
    });
    bus_.systemBus.subscribe(TraderCommand::TradeSpeedDown, [this]() {
      tradeRate_ *= 2;
      Logger::monitorLogger->info(std::format("Trade rate: {}", tradeRate_));
    });

    consoleManager_.addCommand("t+", TraderCommand::TradeStart);
    consoleManager_.addCommand("t-", TraderCommand::TradeStop);
    consoleManager_.addCommand("ts+", TraderCommand::TradeSpeedUp);
    consoleManager_.addCommand("ts-", TraderCommand::TradeSpeedDown);
    consoleManager_.addCommand("q", TraderCommand::Shutdown);
  }

  void start() {
    greetings();
    loadMarketData();
    networkClient_.start();
    consoleManager_.start();

    utils::setTheadRealTime();
    bus_.run();
  }

  void stop() {
    networkClient_.stop();
    consoleManager_.stop();
    bus_.stop();
    Logger::monitorLogger->info("stonk");
  }

private:
  void greetings() {
    Logger::monitorLogger->info("Trader go stonks {}", std::string(38, '~'));
    Logger::monitorLogger->info("Configuration:");
    Config::cfg.logConfig();
    consoleManager_.printCommands();
    Logger::monitorLogger->info(std::string(55, '~'));
  }

  void loadMarketData() {
    prices_ = db::PostgresAdapter::readTickers();
    if (prices_.empty()) {
      throw std::runtime_error("Failed to laod market data");
    }
    Logger::monitorLogger->info(std::format("Market data loaded for {} tickers", prices_.size()));
  }

  void tradeStart() {
    if (!networkClient_.connected()) {
      Logger::monitorLogger->error("Not connected to the server");
      return;
    }
    scheduleTradeTimer();
    scheduleStatsTimer();
  }

  void tradeStop() {
    tradeTimer_.cancel();
    statsTimer_.cancel();
  }

  void onOrderStatus(CRef<OrderStatus> status) {
    spdlog::debug("OrderStatus {}", [&status] { return utils::toString(status); }());
    Tracker::logRtt(status.id);
  }

  void onPriceUpdate(CRef<TickerPrice> price) {
    spdlog::debug([&price] { return utils::toString(price); }());
  }

  void scheduleTradeTimer() {
    tradeTimer_.expires_after(tradeRate_);
    tradeTimer_.async_wait([this](BoostErrorRef ec) {
      if (ec) {
        return;
      }
      tradeSomething();
      scheduleTradeTimer();
    });
  }

  void scheduleStatsTimer() {
    statsTimer_.expires_after(monitorRate_);
    statsTimer_.async_wait([this](BoostErrorRef ec) {
      if (ec) {
        return;
      }
      Tracker::printStats();
      scheduleStatsTimer();
    });
  }

  void tradeSomething() {
    static auto cursor = prices_.begin();
    if (cursor == prices_.end()) {
      cursor = prices_.begin();
    }
    auto tickerPrice = *cursor++;
    Order order;
    order.id = utils::getLinuxTimestamp();
    order.ticker = tickerPrice.ticker;
    order.price = utils::fluctuateThePrice(tickerPrice.price);
    order.action = utils::RNG::rng(1) == 0 ? OrderAction::Buy : OrderAction::Sell;
    order.quantity = utils::RNG::rng(100);
    spdlog::trace("Placing order {}", [&order] { return utils::toString(order); }());

    bus_.marketBus.publish(Span<Order>{&order, 1});
  }

private:
  TraderBus bus_;
  NetworkClient networkClient_;
  TraderConsoleManager consoleManager_;
  std::vector<TickerPrice> prices_;

  SteadyTimer tradeTimer_;
  SteadyTimer statsTimer_;

  Microseconds tradeRate_;
  Seconds monitorRate_;
};
} // namespace hft::trader

#endif // HFT_SERVER_TRADERCONTROLCENTER_HPP
