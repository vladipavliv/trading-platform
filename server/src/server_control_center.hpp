/**
 * @author Vladimir Pavliv
 * @date 2025-03-07
 */

#ifndef HFT_SERVER_SERVERCONTROLCENTER_HPP
#define HFT_SERVER_SERVERCONTROLCENTER_HPP

#include "adapters/adapters.hpp"
#include "authenticator.hpp"
#include "commands/server_command.hpp"
#include "commands/server_command_parser.hpp"
#include "config/server_config.hpp"
#include "console_reader.hpp"
#include "domain_types.hpp"
#include "execution/coordinator.hpp"
#include "network/boost_network/boost_network_server.hpp"
#include "price_feed.hpp"
#include "server_events.hpp"
#include "server_types.hpp"
#include "session_manager.hpp"
#include "storage/storage.hpp"

namespace hft::server {

/**
 * @brief Creates all the components and controls the flow
 */
class ServerControlCenter {
public:
  using SessionManagerType = SessionManager<BoostSessionChannel, BoostBroadcastChannel>;
  using NetworkServerType = BoostNetworkServer<SessionManagerType>;
  using ServerConsoleReader = ConsoleReader<ServerCommandParser>;
  using StreamAdapter = adapters::MessageQueueAdapter<ServerBus, ServerCommandParser>;

  ServerControlCenter()
      : storage_{dbAdapter_}, sessionManager_{bus_}, networkServer_{bus_, sessionManager_},
        authenticator_{bus_.systemBus, dbAdapter_}, coordinator_{bus_, storage_.marketData()},
        consoleReader_{bus_.systemBus}, priceFeed_{bus_, dbAdapter_}, streamAdapter_{bus_} {
    // System bus subscriptions
    bus_.subscribe(ServerEvent::Operational, [this] {
      // start the network server only after internal components are fully operational
      LOG_INFO_SYSTEM("Server is ready");
      networkServer_.start();
    });
    bus_.subscribe(ServerCommand::Shutdown, [this] { stop(); });
  }

  void start() {
    storage_.load();
    if (storage_.marketData().empty()) {
      throw std::runtime_error("No ticker data loaded from db");
    }
    greetings();

    coordinator_.start();
    streamAdapter_.start();
    streamAdapter_.bindProduceTopic<RuntimeMetrics>("runtime-metrics");
    streamAdapter_.bindProduceTopic<OrderTimestamp>("order-timestamps");

    bus_.run();
  }

  void stop() {
    networkServer_.stop();
    sessionManager_.close();
    streamAdapter_.stop();
    coordinator_.stop();
    bus_.stop();
    storage_.save();

    LOG_INFO_SYSTEM("stonk");
  }

private:
  void greetings() {
    LOG_INFO_SYSTEM("Server go stonks");
    LOG_INFO_SYSTEM("Configuration:");
    ServerConfig::log();
    consoleReader_.printCommands();
    LOG_INFO_SYSTEM("Tickers loaded: {}", storage_.marketData().size());
  }

private:
  ServerBus bus_;

  adapters::DbAdapter dbAdapter_;
  Storage storage_;

  SessionManagerType sessionManager_;
  NetworkServerType networkServer_;
  Authenticator authenticator_;
  Coordinator coordinator_;
  ServerConsoleReader consoleReader_;
  PriceFeed priceFeed_;
  StreamAdapter streamAdapter_;
};

} // namespace hft::server

#endif // HFT_SERVER_SERVERCONTROLCENTER_HPP
