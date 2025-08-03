/**
 * @author Vladimir Pavliv
 * @date 2025-03-07
 */

#ifndef HFT_SERVER_SERVERCONTROLCENTER_HPP
#define HFT_SERVER_SERVERCONTROLCENTER_HPP

#include "adapters/kafka/kafka_adapter.hpp"
#include "adapters/postgres/postgres_adapter.hpp"
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
  using Kafka = KafkaAdapter<ServerCommandParser>;

  ServerControlCenter()
      : storage_{dbAdapter_}, sessionManager_{bus_}, networkServer_{bus_, sessionManager_},
        authenticator_{bus_.systemBus, dbAdapter_}, coordinator_{bus_, storage_.marketData()},
        consoleReader_{bus_.systemBus}, priceFeed_{bus_, dbAdapter_}, kafka_{bus_.systemBus} {
    // System bus subscriptions
    bus_.systemBus.subscribe(ServerEvent::Operational, [this] {
      // start the network server only after internal components are fully operational
      LOG_INFO_SYSTEM("Server is ready");
      networkServer_.start();
    });

    // commands
    bus_.systemBus.subscribe(ServerCommand::Shutdown, [this] { stop(); });

    // kafka topics and commands
    kafka_.addConsumeTopic(Config::get<String>("kafka.kafka_server_cmd_topic"));
    kafka_.addProduceTopic<RuntimeMetrics>(Config::get<String>("kafka.kafka_metrics_topic"));
  }

  void start() {
    greetings();
    storage_.load();

    coordinator_.start();
    kafka_.start();
    bus_.run();
  }

  void stop() {
    kafka_.stop();
    networkServer_.stop();
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
    LOG_INFO_SYSTEM("Tickers loaded: {}", storage_.marketData().tickers());
  }

private:
  Bus bus_;

  PostgresAdapter dbAdapter_;
  Storage storage_;

  SessionManagerType sessionManager_;
  NetworkServerType networkServer_;
  Authenticator authenticator_;
  Coordinator coordinator_;
  ServerConsoleReader consoleReader_;
  PriceFeed priceFeed_;
  Kafka kafka_;
};

} // namespace hft::server

#endif // HFT_SERVER_SERVERCONTROLCENTER_HPP
