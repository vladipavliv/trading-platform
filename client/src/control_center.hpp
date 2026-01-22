/**
 * @author Vladimir Pavliv
 * @date 2025-04-06
 */

#ifndef HFT_SERVER_CONTROLCENTER_HPP
#define HFT_SERVER_CONTROLCENTER_HPP

#include <boost/asio/signal_set.hpp>
#include <stop_token>

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
  explicit ControlCenter(ClientConfig &&cfg)
      : config_{std::move(cfg)}, bus_{config_.data}, ctx_{bus_, config_, stopSrc_.get_token()},
        ipcClient_{ctx_}, connectionManager_{ctx_, ipcClient_}, engine_{ctx_},
        consoleReader_{bus_.systemBus}, telemetry_{bus_, config_.data, true},
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
    try {
      ipcClient_.start();
      engine_.start();
      telemetry_.start();

      bus_.run();
    } catch (const std::exception &e) {
      LOG_ERROR_SYSTEM("Exception in CC::start {}", e.what());
    }
  }

  void stop() {
    try {
      stopSrc_.request_stop();

      ipcClient_.stop();
      engine_.stop();
      telemetry_.close();
      connectionManager_.close();
      bus_.stop();

      LOG_INFO_SYSTEM("stonk");
    } catch (const std::exception &e) {
      LOG_ERROR_SYSTEM("Exception in CC::stop {}", e.what());
    }
  }

private:
  void greetings() {
    LOG_INFO_SYSTEM("Client go stonks");
    LOG_INFO_SYSTEM("Configuration:");
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
  const ClientConfig config_;
  std::stop_source stopSrc_;

  ClientBus bus_;
  Context ctx_;

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
