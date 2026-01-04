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
#include "network/boost/boost_network_server.hpp"
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
  using NetworkServer = BoostNetworkServer;
  using StreamTransport = NetworkServer::StreamTransport;
  using DatagramTransport = NetworkServer::DatagramTransport;

  using SessionMgr = SessionManager<StreamTransport>;

  using ConsoleRdr = ConsoleReader<ServerCommandParser>;
  using StreamAdapter = adapters::MessageQueueAdapter<ServerBus, ServerCommandParser>;

  using PricesChannel = Channel<DatagramTransport, ServerBus>;

public:
  ServerControlCenter()
      : storage_{dbAdapter_}, sessionMgr_{bus_}, authenticator_{bus_.systemBus, dbAdapter_},
        coordinator_{bus_, storage_.marketData()}, consoleRdr_{bus_.systemBus},
        priceFeed_{bus_, dbAdapter_}, streamAdapter_{bus_} {
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

    // network callbacks
    networkServer_.setUpstreamClb([this](StreamTransport &&transport) { // format
      sessionMgr_.acceptUpstream(std::move(transport));
    });
    networkServer_.setDownstreamClb([this](StreamTransport &&transport) { // format
      sessionMgr_.acceptDownstream(std::move(transport));
    });
    networkServer_.setDatagramClb([this](DatagramTransport &&transport) {
      const auto id = utils::generateConnectionId();
      LOG_INFO_SYSTEM("UDP prices channel created {}", id);
      pricesChannel_ = std::make_unique<PricesChannel>(std::move(transport), id, bus_);
      bus_.subscribe<TickerPrice>([this](CRef<TickerPrice> p) { pricesChannel_->write(p); });
    });

    // commands
    bus_.systemBus.subscribe(ServerCommand::Shutdown, [this] { stop(); });
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
    LOG_INFO_SYSTEM("stonk");

    running_ = false;

    networkServer_.stop();
    sessionMgr_.close();
    streamAdapter_.stop();
    coordinator_.stop();
    bus_.stop();
    storage_.save();
  }

private:
  void greetings() {
    LOG_INFO_SYSTEM("Server go stonks");
    LOG_INFO_SYSTEM("Configuration:");
    ServerConfig::log();
    consoleRdr_.printCommands();
    LOG_INFO_SYSTEM("Tickers loaded: {}", storage_.marketData().size());
  }

  void startNetwork() {
    workerThreads_.emplace_back([this]() {
      try {
        utils::setTheadRealTime();
        if (ServerConfig::cfg.coreNetwork.has_value()) {
          const size_t coreId = *ServerConfig::cfg.coreNetwork;
          utils::pinThreadToCore(coreId);
          LOG_DEBUG("Started network thread on the core {}", coreId);
        } else {
          LOG_DEBUG("Started network thread");
        }
        networkServer_.start();
        networkLoop();
      } catch (const std::exception &e) {
        LOG_ERROR_SYSTEM("Exception in network thread {}", e.what());
        bus_.post(InternalError(StatusCode::Error, e.what()));
      }
    });
  }

  void networkLoop() {
    while (running_) {
      size_t processed = 0;
      processed += networkServer_.poll();
      processed += sessionMgr_.poll();
      if (processed == 0) {
        __builtin_ia32_pause();
      }
    }
  }

private:
  ServerBus bus_;

  adapters::DbAdapter dbAdapter_;
  Storage storage_;

  SessionMgr sessionMgr_;
  NetworkServer networkServer_;
  Authenticator authenticator_;
  Coordinator coordinator_;
  ConsoleRdr consoleRdr_;
  PriceFeed priceFeed_;
  StreamAdapter streamAdapter_;

  UPtr<PricesChannel> pricesChannel_;

  std::atomic_bool running_{false};
  std::vector<Thread> workerThreads_;
};

} // namespace hft::server

#endif // HFT_SERVER_SERVERCONTROLCENTER_HPP
