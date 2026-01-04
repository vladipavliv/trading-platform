/**
 * @author Vladimir Pavliv
 * @date 2025-04-06
 */

#ifndef HFT_SERVER_CLIENTCONTROLCENTER_HPP
#define HFT_SERVER_CLIENTCONTROLCENTER_HPP

#include "adapters/adapters.hpp"
#include "boost_types.hpp"
#include "client_connection_manager.hpp"
#include "client_events.hpp"
#include "client_types.hpp"
#include "commands/client_command.hpp"
#include "commands/client_command_parser.hpp"
#include "config/client_config.hpp"
#include "console_reader.hpp"
#include "internal_error.hpp"
#include "logging.hpp"
#include "network/boost/boost_network_client.hpp"
#include "trade_engine.hpp"
#include "types.hpp"

namespace hft::client {

/**
 * @brief Creates all the components and controls the flow
 */
class ClientControlCenter {
  using NetworkClient = BoostNetworkClient;
  using StreamTransport = NetworkClient::StreamTransport;
  using DatagramTransport = NetworkClient::DatagramTransport;

  using ConnectionManager = ClientConnectionManager<NetworkClient>;

  using ConsoleRdr = ConsoleReader<ClientCommandParser>;
  using StreamAdapter = adapters::MessageQueueAdapter<ClientBus, ClientCommandParser>;

public:
  ClientControlCenter()
      : connectionManager_{bus_, networkClient_}, engine_{bus_}, streamAdapter_{bus_},
        consoleReader_{bus_.systemBus} {

    bus_.systemBus.subscribe<ClientState>([this](CRef<ClientState> event) {
      LOG_INFO_SYSTEM("{}", utils::toString(event));
      switch (event) {
      case ClientState::Connected:
        LOG_INFO_SYSTEM("Connected to the server");
        break;
      case ClientState::Disconnected:
        LOG_ERROR_SYSTEM("Disconnected from the server");
        stop();
        break;
      case ClientState::InternalError:
        stop();
        break;
      default:
        break;
      }
    });
    bus_.systemBus.subscribe<InternalError>([this](CRef<InternalError> error) {
      LOG_ERROR_SYSTEM("Internal error: {} {}", error.what, utils::toString(error.code));
      stop();
    });

    // commands
    bus_.systemBus.subscribe(ClientCommand::Shutdown, [this]() { stop(); });
  }

  void start() {
    greetings();

    networkClient_.start();
    engine_.start();
    streamAdapter_.start();
    streamAdapter_.bindProduceTopic<RuntimeMetrics>("runtime-metrics");
    streamAdapter_.bindProduceTopic<OrderTimestamp>("order-timestamps");

    LOG_INFO_SYSTEM("Connecting to the server");
    networkClient_.connect();

    bus_.run();
  }

  void stop() {
    LOG_INFO_SYSTEM("stonk");

    engine_.stop();
    streamAdapter_.stop();
    networkClient_.stop();
    bus_.stop();
  }

private:
  void greetings() {
    LOG_INFO_SYSTEM("Client go stonks");
    LOG_INFO_SYSTEM("Configuration:");
    ClientConfig::log();
    consoleReader_.printCommands();
  }

private:
  ClientBus bus_;

  NetworkClient networkClient_;
  ConnectionManager connectionManager_;
  TradeEngine engine_;
  StreamAdapter streamAdapter_;
  ConsoleRdr consoleReader_;
};
} // namespace hft::client

#endif // HFT_SERVER_CLIENTCONTROLCENTER_HPP
