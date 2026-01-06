/**
 * @author Vladimir Pavliv
 * @date 2025-04-06
 */

#ifndef HFT_SERVER_CONTROLCENTER_HPP
#define HFT_SERVER_CONTROLCENTER_HPP

#include "boost_types.hpp"
#include "commands/command.hpp"
#include "commands/command_parser.hpp"
#include "config/client_config.hpp"
#include "connection_manager.hpp"
#include "console_reader.hpp"
#include "events.hpp"
#include "internal_error.hpp"
#include "logging.hpp"
#include "network/boost/boost_network_client.hpp"
#include "network/shm/shm_client.hpp"
#include "trade_engine.hpp"
#include "traits.hpp"
#include "types.hpp"

namespace hft::client {

/**
 * @brief Creates all the components and controls the flow
 */
class ClientControlCenter {
public:
  ClientControlCenter()
      : networkClient_{bus_}, connectionManager_{bus_, networkClient_}, engine_{bus_},
        streamAdapter_{bus_}, consoleReader_{bus_.systemBus} {

    bus_.systemBus.subscribe<ClientState>([this](CRef<ClientState> event) {
      LOG_INFO_SYSTEM("{}", utils::toString(event));
      state_ = event;
      switch (event) {
      case ClientState::Connected:
        break;
      case ClientState::Disconnected:
        stop();
        break;
      case ClientState::InternalError:
        stop();
        break;
      default:
        break;
      }
    });

    bus_.systemBus.subscribe(Command::Start, [this] {
      if (state_ != ClientState::Connected) {
        LOG_ERROR_SYSTEM("Not connected to the server");
        return;
      }
      engine_.tradeStart();
    });
    bus_.systemBus.subscribe(Command::Stop, [this] { engine_.tradeStop(); });

    bus_.systemBus.subscribe<InternalError>([this](CRef<InternalError> error) {
      LOG_ERROR_SYSTEM("Internal error: {} {}", error.what, utils::toString(error.code));
      stop();
    });

    // commands
    bus_.systemBus.subscribe(Command::Shutdown, [this]() { stop(); });
  }

  void start() {
    greetings();

    networkClient_.start();
    engine_.start();
    streamAdapter_.start();
    streamAdapter_.bindProduceTopic<RuntimeMetrics>("runtime-metrics");
    streamAdapter_.bindProduceTopic<OrderTimestamp>("order-timestamps");

    bus_.run();
  }

  void stop() {
    LOG_INFO_SYSTEM("stonk");

    connectionManager_.close();
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
  ClientConsoleReader consoleReader_;

  ClientState state_{ClientState::Disconnected};
};
} // namespace hft::client

#endif // HFT_SERVER_CONTROLCENTER_HPP
