/**
 * @author Vladimir Pavliv
 * @date 2025-03-07
 */

#ifndef HFT_SERVER_TRADERCONTROLCENTER_HPP
#define HFT_SERVER_TRADERCONTROLCENTER_HPP

#include <atomic>
#include <vector>

#include "boost_types.hpp"
#include "bus/bus.hpp"
#include "bus/subscription_holder.hpp"
#include "config/config.hpp"
#include "console_reader.hpp"
#include "db/postgres_adapter.hpp"
#include "network/network_client.hpp"
#include "rtt_tracker.hpp"
#include "trader_command.hpp"
#include "trader_events.hpp"
#include "types.hpp"
#include "utils/market_utils.hpp"
#include "utils/rng.hpp"
#include "utils/template_utils.hpp"

namespace hft::trader {

/**
 * @brief Starts all the components and controls the flow
 * Runs the system io_context, executes console commands, reacts to system events
 * At the moment is simplified to runs trading in the system io_context which is not ideal
 * Trading bottlenecks at 150k rps with 1us trade timer, tested additionally - verified that
 * generating order, serializing, allocating buffer for async socket write, copying serialized
 * data and sending it asynchronously - it all does take ~5-7us
 * @todo Try protobuf and Cap'n Proto which might be better and more compact for smol messages
 */
class TraderControlCenter {
public:
  using UPtr = std::unique_ptr<TraderControlCenter>;
  using TraderConsoleReader = ConsoleReader<TraderCommand>;
  using Tracker = RttTracker<50>;

  TraderControlCenter()
      : networkClient_{bus_}, consoleReader_{bus_.systemBus}, dbAdapter_{bus_.systemBus},
        tradeTimer_{bus_.systemCtx()}, statsTimer_{bus_.systemCtx()},
        tradeRate_{Config::cfg.tradeRate}, monitorRate_{Config::cfg.monitorRate} {

    bus_.marketBus.setHandler<OrderStatus>([this](CRef<OrderStatus> s) { onOrderStatus(s); });
    bus_.marketBus.setHandler<TickerPrice>([this](CRef<TickerPrice> p) { onPriceUpdate(p); });

    // System events
    bus_.systemBus.subscribe<LoginResponse>([this](CRef<LoginResponse> l) { onLoginResponse(l); });

    bus_.systemBus.subscribe(TraderEvent::Connected, [this] { LOG_INFO_SYSTEM("Connected"); });
    bus_.systemBus.subscribe(TraderEvent::Disconnected, [this] {
      LOG_ERROR_SYSTEM("Disconnected");
      tradeStop();
    });

    // Console commands
    bus_.systemBus.subscribe(TraderCommand::Shutdown, [this] { stop(); });
    bus_.systemBus.subscribe(TraderCommand::TradeStart, [this] { tradeStart(); });
    bus_.systemBus.subscribe(TraderCommand::TradeStop, [this] { tradeStop(); });
    bus_.systemBus.subscribe(TraderCommand::TradeSpeedUp, [this] {
      if (tradeRate_ > Microseconds(1)) {
        tradeRate_ /= 2;
        LOG_INFO_SYSTEM("Trade rate: {}", tradeRate_.count());
      }
    });
    bus_.systemBus.subscribe(TraderCommand::TradeSpeedDown, [this] {
      tradeRate_ *= 2;
      LOG_INFO_SYSTEM("Trade rate: {}", tradeRate_.count());
    });

    consoleReader_.addCommand("t+", TraderCommand::TradeStart);
    consoleReader_.addCommand("t-", TraderCommand::TradeStop);
    consoleReader_.addCommand("ts+", TraderCommand::TradeSpeedUp);
    consoleReader_.addCommand("ts-", TraderCommand::TradeSpeedDown);
    consoleReader_.addCommand("q", TraderCommand::Shutdown);
  }

  void start() {
    greetings();
    loadMarketData();
    networkClient_.start();
    consoleReader_.start();

    utils::setTheadRealTime(Config::cfg.coreSystem);
    utils::pinThreadToCore(Config::cfg.coreSystem);

    scheduleStatsTimer();
    bus_.systemCtx().run();
  }

  void stop() {
    networkClient_.stop();
    consoleReader_.stop();
    bus_.systemCtx().stop();
    LOG_INFO_SYSTEM("stonk");
  }

private:
  void greetings() {
    LOG_INFO_SYSTEM("Trader go stonks {}", std::string(38, '~'));
    LOG_INFO_SYSTEM("Configuration:");
    Config::cfg.logConfig();
    consoleReader_.printCommands();
    LOG_INFO_SYSTEM("{}", std::string(55, '~'));
  }

  void loadMarketData() {
    prices_ = dbAdapter_.readTickers();
    if (prices_.empty()) {
      throw std::runtime_error("Failed to laod market data");
    }
    LOG_DEBUG("Market data loaded for {} tickers", prices_.size());
  }

  void tradeStart() {
    LOG_INFO_SYSTEM("Trade start");
    if (!networkClient_.connected()) {
      LOG_ERROR_SYSTEM("Not connected to the server");
      return;
    }
    trading_ = true;
    scheduleTradeTimer();
  }

  void tradeStop() {
    LOG_INFO_SYSTEM("Trade stop");
    tradeTimer_.cancel();
    trading_ = false;
  }

  void onOrderStatus(CRef<OrderStatus> status) {
    LOG_DEBUG("{}", utils::toString(status));
    Tracker::logRtt(status.timestamp);
  }

  void onPriceUpdate(CRef<TickerPrice> price) {
    LOG_DEBUG("{}", utils::toString(price));
    pricesCounter_.fetch_add(1, std::memory_order_relaxed);
  }

  void onLoginResponse(CRef<LoginResponse> response) {
    LOG_DEBUG("{}", utils::toString(response));
    if (!response.success) {
      LOG_ERROR_SYSTEM("Failed to log in")
    } else {
      LOG_INFO_SYSTEM("Logged in successfully");
      token_ = response.token;
    }
  }

  void scheduleTradeTimer() {
    if (!trading_) {
      return;
    }
    if (!token_.has_value()) {
      LOG_ERROR_SYSTEM("Not logged in");
      return;
    }
    tradeTimer_.expires_after(tradeRate_);
    tradeTimer_.async_wait([this](CRef<BoostError> ec) {
      if (ec) {
        return;
      }
      tradeSomething();
      scheduleTradeTimer();
    });
  }

  void scheduleStatsTimer() {
    if (monitorRate_.count() == 0) {
      return;
    }
    statsTimer_.expires_after(monitorRate_);
    statsTimer_.async_wait([this](CRef<BoostError> ec) {
      if (ec) {
        return;
      }
      using namespace utils;
      static size_t requestsLast = 0;
      static size_t pricesCounterLast = 0;

      size_t requestsCurrent = requestCounter_.load(std::memory_order_relaxed);
      size_t rps = (requestsCurrent - requestsLast) / monitorRate_.count();

      size_t pricesCounterCurrent = pricesCounter_.load(std::memory_order_relaxed);
      size_t prps = (pricesCounterCurrent - pricesCounterLast) / monitorRate_.count();

      std::string output;
      if (rps != 0) {
        output += std::format("Rtt: {} | Trade rps: {}", Tracker::getRttStats(), thousandify(rps));
      }
      if (prps != 0) {
        if (!output.empty()) {
          output += " | ";
        }
        output += std::format("Prices rps: {}", thousandify(prps));
      }
      if (!output.empty()) {
        LOG_INFO_SYSTEM("{}", output);

        requestsLast = requestsCurrent;
        pricesCounterLast = pricesCounterCurrent;
      }
      scheduleStatsTimer();
    });
  }

  void tradeSomething() {
    static auto cursor = prices_.begin();
    if (cursor == prices_.end()) {
      cursor = prices_.begin();
    }
    auto p = *cursor++;
    auto newPrice = utils::fluctuateThePrice(p.price);
    auto action = utils::RNG::rng<uint8_t>(1) == 0 ? OrderAction::Buy : OrderAction::Sell;
    auto quantity = utils::RNG::rng<Quantity>(100);
    // TODO(self) set all the fields
    Order order{0, 0, 0, utils::getTimestamp(), p.ticker, quantity, newPrice, action};
    LOG_DEBUG("Placing order {}", utils::toString(order));
    bus_.marketBus.post(order);
    requestCounter_.fetch_add(1, std::memory_order_relaxed);
  }

private:
  Bus bus_;
  NetworkClient networkClient_;
  TraderConsoleReader consoleReader_;
  PostgresAdapter dbAdapter_;
  std::vector<TickerPrice> prices_;

  SteadyTimer tradeTimer_;
  SteadyTimer statsTimer_;

  Microseconds tradeRate_;
  const Seconds monitorRate_;

  std::atomic_size_t requestCounter_{0};
  std::atomic_size_t pricesCounter_{0};
  std::atomic_bool trading_{false};
};
} // namespace hft::trader

#endif // HFT_SERVER_TRADERCONTROLCENTER_HPP
