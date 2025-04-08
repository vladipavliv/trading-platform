/**
 * @author Vladimir Pavliv
 * @date 2025-03-07
 */

#ifndef HFT_SERVER_SERVERCONTROLCENTER_HPP
#define HFT_SERVER_SERVERCONTROLCENTER_HPP

#include "adapters/kafka/kafka_adapter.hpp"
#include "adapters/postgres/postgres_adapter.hpp"
#include "bus/bus.hpp"
#include "config/config.hpp"
#include "config/config_reader.hpp"
#include "console_reader.hpp"
#include "coordinator.hpp"
#include "market_types.hpp"
#include "network/network_server.hpp"
#include "price_feed.hpp"
#include "server_command.hpp"
#include "server_events.hpp"

namespace hft::server {

/**
 * @brief Creates all the components and sets up console commands
 */
class ServerControlCenter {
public:
  using UPtr = std::unique_ptr<ServerControlCenter>;
  using ServerConsoleReader = ConsoleReader<ServerCommand>;

  ServerControlCenter()
      : dbAdapter_{bus_.systemBus}, networkServer_{bus_}, coordinator_{bus_, marketData_},
        consoleReader_{bus_.systemBus}, priceFeed_{bus_, marketData_}, kafka_{bus_.systemBus} {

    // System bus subscriptions
    bus_.systemBus.subscribe<ServerEvent>(ServerEvent::Ready, [this] {
      LOG_INFO_SYSTEM("Server is ready");
      networkServer_.start();
    });
    bus_.systemBus.subscribe<ServerCommand>(ServerCommand::Shutdown, [this] { stop(); });

    // Console commands
    consoleReader_.addCommand("q", ServerCommand::Shutdown);
    consoleReader_.addCommand("p+", ServerCommand::PriceFeedStart);
    consoleReader_.addCommand("p-", ServerCommand::PriceFeedStop);
  }

  void start() {
    greetings();
    readMarketData();

    coordinator_.start();
    consoleReader_.start();

    utils::setTheadRealTime();
    if (Config::cfg.coreSystem.has_value()) {
      utils::pinThreadToCore(Config::cfg.coreSystem.value());
    }

    bus_.systemBus.ioCtx.run();
  }

  void stop() {
    networkServer_.stop();
    coordinator_.stop();
    consoleReader_.stop();
    bus_.systemBus.ioCtx.stop();
    LOG_INFO_SYSTEM("stonk");
  }

private:
  void greetings() {
    LOG_INFO_SYSTEM("Server go stonks");
    LOG_INFO_SYSTEM("Configuration:");
    Config::cfg.logConfig();
    consoleReader_.printCommands();
  }

  void readMarketData() {
    auto prices = dbAdapter_.readTickers();

    const auto workers = Config::cfg.coresApp.size();
    ThreadId roundRobin = 0;
    for (auto &item : prices) {
      marketData_.emplace(item.ticker, std::make_unique<TickerData>(roundRobin, item.price));
      roundRobin = (++roundRobin < workers) ? roundRobin : 0;
    }
    LOG_INFO_SYSTEM("Market data loaded for {} tickers", marketData_.size());
  }

private:
  Bus bus_;

  PostgresAdapter dbAdapter_;
  NetworkServer networkServer_;
  Coordinator coordinator_;
  ServerConsoleReader consoleReader_;
  PriceFeed priceFeed_;
  KafkaAdapter<> kafka_;

  MarketData marketData_;
};

} // namespace hft::server

#endif // HFT_SERVER_SERVERCONTROLCENTER_HPP
