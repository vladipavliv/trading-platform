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
#include "network/network_server.hpp"
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
  static constexpr size_t POLL_CHUNK = 16;

public:
  using SessionManagerType = SessionManager<SessionChannel, BroadcastChannel>;
  using NetworkServerType = NetworkServer<SessionManagerType>;
  using ServerConsoleReader = ConsoleReader<ServerCommandParser>;
  using StreamAdapter = adapters::MessageQueueAdapter<ServerBus, ServerCommandParser>;

  ServerControlCenter()
      : storage_{dbAdapter_}, sessionManager_{bus_}, networkServer_{bus_, sessionManager_},
        authenticator_{bus_.systemBus, dbAdapter_}, coordinator_{bus_, storage_.marketData()},
        consoleReader_{bus_.systemBus}, priceFeed_{bus_, dbAdapter_}, streamAdapter_{bus_} {
    // System bus subscriptions
    bus_.subscribe<ServerEvent>([this](CRef<ServerEvent> event) {
      switch (event.state) {
      case ServerState::Operational:
        // start the network server only after internal components are fully operational
        LOG_INFO_SYSTEM("Server is ready");
        startNetwork();
        break;
      default:
        break;
      }
    });
    bus_.subscribe<InternalError>([this](CRef<InternalError> error) {
      LOG_ERROR_SYSTEM("Internal error: {} {}", error.what, utils::toString(error.code));
      stop();
    });
    bus_.subscribe(ServerCommand::Shutdown, [this] { stop(); });
  }

  void start() {
    running_ = true;
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
    running_ = false;

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

  void startNetwork() {
    const auto addThread = [this](uint8_t workerId, Optional<CoreId> coreId = Optional<CoreId>{}) {
      workerThreads_.emplace_back([this, workerId, coreId]() {
        try {
          utils::setTheadRealTime();
          if (coreId.has_value()) {
            utils::pinThreadToCore(coreId.value());
            LOG_DEBUG("Worker {} started on the core {}", workerId, coreId.value());
          } else {
            LOG_DEBUG("Worker {} started", workerId);
          }
          networkLoop();
        } catch (const std::exception &e) {
          LOG_ERROR_SYSTEM("Exception in network thread {}", e.what());
          bus_.post(InternalError(StatusCode::Error, e.what()));
        }
      });
    };
    const auto cores = ServerConfig::cfg.coresNetwork.size();
    workerThreads_.reserve(cores == 0 ? 1 : cores);
    if (cores == 0) {
      addThread(0);
    }
    for (size_t i = 0; i < cores; ++i) {
      addThread(i, ServerConfig::cfg.coresNetwork[i]);
    }
  }

  void networkLoop() {
    while (running_) {
      for (int i = 0; i < POLL_CHUNK; ++i) {
        networkServer_.pollOne();
      }
      sessionManager_.poll();
    }
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

  std::atomic_bool running_{false};
  std::vector<Thread> workerThreads_;
};

} // namespace hft::server

#endif // HFT_SERVER_SERVERCONTROLCENTER_HPP
