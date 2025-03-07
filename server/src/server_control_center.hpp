/**
 * @author Vladimir Pavliv
 * @date 2025-03-07
 */

#ifndef HFT_SERVER_SERVERCONTROLCENTER_HPP
#define HFT_SERVER_SERVERCONTROLCENTER_HPP

#include "config/config.hpp"
#include "config/config_reader.hpp"
#include "console_manager.hpp"
#include "coordinator.hpp"
#include "db/postgres_adapter.hpp"
#include "market_types.hpp"
#include "network_server.hpp"
#include "price_feed.hpp"
#include "server_bus.hpp"
#include "server_command.hpp"
#include "server_events.hpp"

namespace hft::server {

/**
 * @brief Starts all the components and controls the flow
 * Runs the system io_context and performs higher level management
 */
class ServerControlCenter {
public:
  using UPtr = std::unique_ptr<ServerControlCenter>;
  using ServerConsoleManager = ConsoleManager<ServerCommand>;

  ServerControlCenter()
      : ctxGuard_{boost::asio::make_work_guard(systemIoCtx_)}, bus_{systemIoCtx_},
        networkServer_{bus_}, coordinator_{bus_, marketData_}, consoleManager_{bus_.systemBus},
        priceFeed_{bus_, marketData_} {

    // System bus subscriptions
    bus_.systemBus.subscribe(ServerCommand::Shutdown, [this]() { stop(); });
    bus_.systemBus.subscribe(ServerEvent::CoresWarmedUp, [this]() {
      Logger::monitorLogger->info("Workers warmed up");
      networkServer_.start();
    });

    // Console commands
    consoleManager_.addCommand("q", ServerCommand::Shutdown);
    consoleManager_.addCommand("p+", ServerCommand::PriceFeedStart);
    consoleManager_.addCommand("p-", ServerCommand::PriceFeedStop);
  }

  void start() {
    greetings();
    readMarketData();
    coordinator_.start();
    consoleManager_.start();

    utils::setTheadRealTime();
    systemIoCtx_.run();
  }

  void stop() {
    networkServer_.stop();
    coordinator_.stop();
    consoleManager_.stop();
    systemIoCtx_.stop();
    Logger::monitorLogger->info("stonk");
  }

private:
  void greetings() {
    Logger::monitorLogger->info("Server go stonks {}", std::string(38, '~'));
    Logger::monitorLogger->info("Configuration:");
    Config::cfg.logConfig();
    consoleManager_.printCommands();
    Logger::monitorLogger->info(std::string(55, '~'));
  }

  void readMarketData() {
    auto prices = db::PostgresAdapter::readTickers();

    const auto workers = Config::cfg.coresApp.size();
    ThreadId roundRobin = 0;
    for (auto &item : prices) {
      marketData_.emplace(item.ticker, std::make_unique<TickerData>(roundRobin, item.price));
      roundRobin = (++roundRobin < workers) ? roundRobin : 0;
    }
    Logger::monitorLogger->info(
        std::format("Market data loaded for {} tickers", marketData_.size()));
  }

private:
  IoContext systemIoCtx_;
  ContextGuard ctxGuard_;

  ServerBus bus_;
  MarketData marketData_;

  NetworkServer networkServer_;
  Coordinator coordinator_;
  ServerConsoleManager consoleManager_;
  PriceFeed priceFeed_;
};
} // namespace hft::server

#endif // HFT_SERVER_SERVERCONTROLCENTER_HPP
