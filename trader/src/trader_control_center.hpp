/**
 * @author Vladimir Pavliv
 * @date 2025-04-06
 */

#ifndef HFT_SERVER_TRADERCONTROLCENTER_HPP
#define HFT_SERVER_TRADERCONTROLCENTER_HPP

#include "adapters/kafka/kafka_adapter.hpp"
#include "boost_types.hpp"
#include "bus/bus.hpp"
#include "config/config.hpp"
#include "console_reader.hpp"
#include "logging.hpp"
#include "network/network_client.hpp"
#include "trader_command.hpp"
#include "trader_engine.hpp"
#include "trader_events.hpp"
#include "types.hpp"

namespace hft::trader {

/**
 * @brief Creates all the components and sets up console commands
 */
class TraderControlCenter {
public:
  using UPtr = std::unique_ptr<TraderControlCenter>;

  TraderControlCenter()
      : networkClient_{bus_}, engine_{bus_}, kafka_{bus_.systemBus}, consoleReader_{bus_.systemBus},
        timer_{bus_.systemCtx()} {
    bus_.systemBus.subscribe(TraderCommand::Shutdown, [this]() { stop(); });

    bus_.systemBus.subscribe(TraderEvent::ConnectedToTheServer, [this]() {
      networkConnected_ = true;
      timer_.cancel();
    });
    bus_.systemBus.subscribe(TraderEvent::DisconnectedFromTheServer, [this]() {
      networkConnected_ = false;
      scheduleReconnect();
    });

    consoleReader_.addCommand("t+", TraderCommand::TradeStart);
    consoleReader_.addCommand("t-", TraderCommand::TradeStop);
    consoleReader_.addCommand("ts+", TraderCommand::TradeSpeedUp);
    consoleReader_.addCommand("ts-", TraderCommand::TradeSpeedDown);
    consoleReader_.addCommand("q", TraderCommand::Shutdown);
  }

  void start() {
    greetings();

    networkClient_.start();
    engine_.start();
    consoleReader_.start();

    LOG_INFO_SYSTEM("Connecting to the server");
    networkClient_.connect();

    utils::setTheadRealTime();
    if (Config::cfg.coreSystem.has_value()) {
      utils::pinThreadToCore(Config::cfg.coreSystem.value());
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
    LOG_INFO_SYSTEM("Trader go stonks");
    LOG_INFO_SYSTEM("Configuration:");
    Config::cfg.logConfig();
    consoleReader_.printCommands();
  }

  void scheduleReconnect() {
    if (networkConnected_) {
      return;
    }
    LOG_ERROR_SYSTEM("Server is down, reconnecting...");
    timer_.expires_after(Config::cfg.monitorRate);
    timer_.async_wait([this](CRef<BoostError> ec) {
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
  TraderEngine engine_;
  KafkaAdapter<> kafka_;
  ConsoleReader<TraderCommand> consoleReader_;

  std::atomic_bool networkConnected_{false};
  SteadyTimer timer_;
};
} // namespace hft::trader

#endif // HFT_SERVER_TRADERCONTROLCENTER_HPP
