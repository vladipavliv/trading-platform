/**
 * @author Vladimir Pavliv
 * @date 2025-03-07
 */

#ifndef HFT_SERVER_SERVERCONTROLCENTER_HPP
#define HFT_SERVER_SERVERCONTROLCENTER_HPP

#include "bus/bus.hpp"
#include "bus/subscription_holder.hpp"
#include "config/config.hpp"
#include "config/config_reader.hpp"
#include "console_reader.hpp"
#include "coordinator.hpp"
#include "db/postgres_adapter.hpp"
#include "market_types.hpp"
#include "network/network_server.hpp"
#include "price_feed.hpp"
#include "server_command.hpp"
#include "server_events.hpp"

namespace hft::server {

/**
 * @brief Starts all the components and controls the flow
 * Runs the system io_context, executes console commands, reacts to system events
 * @todo Later on would do finer startup stage management and performance monitoring
 */
class ServerControlCenter {
public:
  using UPtr = std::unique_ptr<ServerControlCenter>;
  using ServerConsoleReader = ConsoleReader<ServerCommand>;

  ServerControlCenter()
      : networkServer_{bus_}, coordinator_{bus_, marketData_}, consoleReader_{bus_.systemBus},
        priceFeed_{bus_, marketData_}, subs_{id_, bus_.systemBus} {

    // System bus subscriptions
    bus_.systemBus.subscribe<ServerEvent>(
        id_, ServerEvent::Ready,
        subs_.add<CRefHandler<ServerEvent>>([this](CRef<ServerEvent> event) {
          Logger::monitorLogger->info("Server is ready");
          networkServer_.start();
        }));
    bus_.systemBus.subscribe<ServerCommand>(
        id_, ServerCommand::Shutdown,
        subs_.add<CRefHandler<ServerCommand>>([this](CRef<ServerCommand> event) { stop(); }));

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

    utils::setTheadRealTime(Config::cfg.coreSystem);
    utils::pinThreadToCore(Config::cfg.coreSystem);
    bus_.systemBus.ioCtx.run();
  }

  void stop() {
    networkServer_.stop();
    coordinator_.stop();
    consoleReader_.stop();
    bus_.systemBus.ioCtx.stop();
    Logger::monitorLogger->info("stonk");
  }

private:
  void greetings() {
    Logger::monitorLogger->info("Server go stonks {}", std::string(38, '~'));
    Logger::monitorLogger->info("Configuration:");
    Config::cfg.logConfig();
    consoleReader_.printCommands();
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
    Logger::monitorLogger->info("Market data loaded for {} tickers", marketData_.size());
  }

private:
  const ObjectId id_{utils::getId()};

  Bus bus_;
  MarketData marketData_;

  NetworkServer networkServer_;
  Coordinator coordinator_;
  ServerConsoleReader consoleReader_;
  PriceFeed priceFeed_;

  SubscriptionHolder subs_;
};

} // namespace hft::server

#endif // HFT_SERVER_SERVERCONTROLCENTER_HPP
