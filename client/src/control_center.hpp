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
#include "execution/trade_engine.hpp"
#include "internal_error.hpp"
#include "logging.hpp"
#include "primitive_types.hpp"
#include "traits.hpp"
#include "utils/console_reader.hpp"
#include "utils/handler.hpp"

namespace hft::client {

/**
 * @brief Creates all the components and controls the flow
 */
class ControlCenter {
  using SelfT = ControlCenter;

public:
  explicit ControlCenter(ClientConfig &&cfg)
      : config_{std::move(cfg)}, bus_{config_.data}, ctx_{bus_, config_, stopSrc_.get_token()},
        ipcClient_{ctx_}, connectionManager_{ctx_, ipcClient_}, engine_{ctx_},
        consoleReader_{bus_.systemBus}, telemetry_{bus_, config_.data, true},
        signals_{bus_.systemIoCtx(), SIGINT, SIGTERM} {

    bus_.subscribe(CRefHandler<ComponentReady>::bind<SelfT, &SelfT::post>(this));
    bus_.subscribe(CRefHandler<ServerConnectionState>::bind<SelfT, &SelfT::post>(this));
    bus_.subscribe(CRefHandler<InternalError>::bind<SelfT, &SelfT::post>(this));

    bus_.subscribe(Command::Start, Callback::bind<SelfT, &SelfT::tradeStart>(this));
    bus_.subscribe(Command::Stop, Callback::bind<SelfT, &SelfT::tradeStop>(this));
    bus_.subscribe(Command::Shutdown, Callback::bind<SelfT, &SelfT::stop>(this));

    signals_.async_wait([&](BoostErrorCode code, int) {
      LOG_INFO_SYSTEM("Signal received {}, stopping...", code.message());
      stop();
    });

    bus_.post([this]() {
      config_.nsPerCycle = utils::getNsPerCycle();
      bus_.post(ComponentReady(Component::Time));
    });
  }

  ~ControlCenter() { LOG_DEBUG_SYSTEM("~ControlCenter"); }

  void start() {
    greetings();
    try {
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
    config_.print();
    consoleReader_.printCommands();
  }

  void post(CRef<ComponentReady> event) {
    LOG_INFO_SYSTEM("ComponentReady {}", toString(event.id));
    readyMask_.fetch_or((uint8_t)event.id, std::memory_order_relaxed);
    const auto mask = readyMask_.load(std::memory_order_relaxed);
    if (mask == INTERNAL_READY) {
      ipcClient_.start();
    } else if (mask == ALL_READY) {
      LOG_INFO_SYSTEM("Client started");
    }
  }

  void post(CRef<ServerConnectionState> event) {
    LOG_INFO_SYSTEM("{}", toString(event));
    state_ = event;
    switch (event) {
    case ServerConnectionState::Disconnected:
      stop();
      break;
    case ServerConnectionState::Connected:
      break;
    case ServerConnectionState::Failed:
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
    if (state_ != ServerConnectionState::Connected) {
      LOG_ERROR_SYSTEM("Not connected to the server");
      return;
    }
    engine_.tradeStart();
  }

  void tradeStop() { engine_.tradeStop(); }

private:
  ClientConfig config_;
  std::stop_source stopSrc_;

  ClientBus bus_;
  Context ctx_;

  IpcClient ipcClient_;
  ConnectionManager connectionManager_;
  TradeEngine engine_;
  ClientConsoleReader consoleReader_;
  ClientTelemetry telemetry_;

  Atomic<uint8_t> readyMask_;
  ServerConnectionState state_{ServerConnectionState::Disconnected};

  boost::asio::signal_set signals_;
};
} // namespace hft::client

#endif // HFT_SERVER_CONTROLCENTER_HPP
