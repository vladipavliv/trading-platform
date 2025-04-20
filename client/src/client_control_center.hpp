/**
 * @author Vladimir Pavliv
 * @date 2025-04-06
 */

#ifndef HFT_SERVER_CLIENTCONTROLCENTER_HPP
#define HFT_SERVER_CLIENTCONTROLCENTER_HPP

#include "adapters/kafka/kafka_adapter.hpp"
#include "adapters/postgres/postgres_adapter.hpp"
#include "boost_types.hpp"
#include "client_command.hpp"
#include "client_command_parser.hpp"
#include "client_events.hpp"
#include "client_types.hpp"
#include "config/client_config.hpp"
#include "console_reader.hpp"
#include "logging.hpp"
#include "network/network_client.hpp"
#include "trade_engine.hpp"
#include "types.hpp"

namespace hft::client {

/**
 * @brief Creates all the components and controls the flow
 */
class ClientControlCenter {
public:
  using ClientConsoleReader = ConsoleReader<ClientCommandParser>;
  using Kafka = KafkaAdapter<ClientCommandParser>;

  ClientControlCenter()
      : networkClient_{bus_}, engine_{bus_}, kafka_{bus_.systemBus}, consoleReader_{bus_.systemBus},
        timer_{bus_.systemCtx()} {

    bus_.systemBus.subscribe<ClientEvent>([this](CRef<ClientEvent> event) {
      switch (event) {
      case ClientEvent::Connected:
        LOG_INFO_SYSTEM("Connected to the server");
        networkConnected_ = true;
        timer_.cancel();
        break;
      case ClientEvent::Disconnected:
      case ClientEvent::ConnectionFailed:
      case ClientEvent::InternalError:
        networkConnected_ = false;
        scheduleReconnect();
        break;
      default:
        break;
      }
    });

    // commands
    bus_.systemBus.subscribe(ClientCommand::Shutdown, [this]() { stop(); });
    bus_.systemBus.subscribe(ClientCommand::KafkaFeedStart, [this]() {
      LOG_INFO_SYSTEM("Start kafka feed");
      kafka_.start();
    });
    bus_.systemBus.subscribe(ClientCommand::KafkaFeedStop, [this]() {
      LOG_INFO_SYSTEM("Stop kafka feed");
      kafka_.stop();
    });

    // kafka topics
    kafka_.addProduceTopic<OrderTimestamp>("order-timestamps");
    kafka_.addConsumeTopic("client-commands");
  }

  void start() {
    greetings();

    networkClient_.start();
    engine_.start();
    consoleReader_.start();

    LOG_INFO_SYSTEM("Connecting to the server");
    networkClient_.connect();

    utils::setTheadRealTime();
    if (ClientConfig::cfg.coreSystem.has_value()) {
      utils::pinThreadToCore(ClientConfig::cfg.coreSystem.value());
    }
    bus_.systemCtx().run();
  }

  void stop() {
    networkClient_.stop();
    consoleReader_.stop();
    engine_.stop();

    bus_.systemCtx().stop();
    LOG_INFO_SYSTEM("stonk");
  }

private:
  void greetings() {
    LOG_INFO_SYSTEM("Client go stonks");
    LOG_INFO_SYSTEM("Configuration:");
    ClientConfig::log();
    consoleReader_.printCommands();
  }

  void scheduleReconnect() {
    if (networkConnected_) {
      return;
    }
    LOG_ERROR_SYSTEM("Server is down, reconnecting...");
    timer_.expires_after(ClientConfig::cfg.monitorRate);
    timer_.async_wait([this](BoostErrorCode ec) {
      if (ec) {
        LOG_ERROR("{}", ec.message());
        return;
      }
      networkClient_.connect();
    });
  }

private:
  Bus bus_;

  NetworkClient networkClient_;
  TradeEngine engine_;
  Kafka kafka_;
  ClientConsoleReader consoleReader_;

  std::atomic_bool networkConnected_{false};
  SteadyTimer timer_;
};
} // namespace hft::client

#endif // HFT_SERVER_CLIENTCONTROLCENTER_HPP
