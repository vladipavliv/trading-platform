/**
 * @author Vladimir Pavliv
 * @date 2025-04-06
 */

#ifndef HFT_SERVER_CONTROLCENTER_HPP
#define HFT_SERVER_CONTROLCENTER_HPP

#include <boost/asio/signal_set.hpp>

#include "adapters/telemetry_adapter.hpp"
#include "commands/command.hpp"
#include "commands/command_parser.hpp"
#include "config/client_config.hpp"
#include "events.hpp"
#include "internal_error.hpp"
#include "logging.hpp"
#include "primitive_types.hpp"
#include "trade_engine.hpp"
#include "traits.hpp"
#include "utils/console_reader.hpp"

namespace hft::client {

/**
 * @brief Creates all the components and controls the flow
 */
class ControlCenter {
public:
  ControlCenter()
      : ipcClient_{bus_}, connectionManager_{bus_, ipcClient_}, engine_{bus_},
        consoleReader_{bus_.systemBus}, telemetry_{bus_, true},
        signals_{bus_.systemIoCtx(), SIGINT, SIGTERM} {

    using SelfT = ControlCenter;
    bus_.systemBus.subscribe(Command::Start,
                             Callback::template bind<SelfT, &SelfT::tradeStart>(this));
    bus_.systemBus.subscribe(Command::Stop,
                             Callback::template bind<SelfT, &SelfT::tradeStop>(this));
    bus_.systemBus.subscribe(Command::Shutdown, Callback::template bind<SelfT, &SelfT::stop>(this));

    bus_.systemBus.subscribe<ClientState>(
        CRefHandler<ClientState>::template bind<SelfT, &SelfT::post>(this));
    bus_.systemBus.subscribe<InternalError>(
        CRefHandler<InternalError>::template bind<SelfT, &SelfT::post>(this));

    signals_.async_wait([&](BoostErrorCode code, int) {
      LOG_INFO_SYSTEM("Signal received {}, stopping...", code.message());
      stop();
    });
  }

  ~ControlCenter() { LOG_DEBUG_SYSTEM("~ControlCenter"); }

  void start() {
    greetings();

    ipcClient_.start();
    engine_.start();
    telemetry_.start();

    bus_.run();
  }

  void stop() {
    LOG_INFO_SYSTEM("stonk");

    ipcClient_.stop();
    engine_.stop();
    telemetry_.close();
    connectionManager_.close();
    bus_.stop();
  }

private:
  void greetings() {
    LOG_INFO_SYSTEM("Client go stonks");
    LOG_INFO_SYSTEM("Configuration:");
    ClientConfig::cfg().log();
    consoleReader_.printCommands();
  }

  void post(CRef<ClientState> event) {
    LOG_INFO_SYSTEM("{}", toString(event));
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
  }

  void post(CRef<InternalError> error) {
    LOG_ERROR_SYSTEM("Internal error: {} {}", error.what, toString(error.code));
    stop();
  }

  void tradeStart() {
    if (state_ != ClientState::Connected) {
      LOG_ERROR_SYSTEM("Not connected to the server");
      return;
    }
    engine_.tradeStart();
  }

  void tradeStop() { engine_.tradeStop(); }

private:
  ClientBus bus_;

  IpcClient ipcClient_;
  ConnectionManager connectionManager_;
  TradeEngine engine_;
  ClientConsoleReader consoleReader_;
  ClientTelemetry telemetry_;

  ClientState state_{ClientState::Disconnected};

  boost::asio::signal_set signals_;
};
} // namespace hft::client

#endif // HFT_SERVER_CONTROLCENTER_HPP
