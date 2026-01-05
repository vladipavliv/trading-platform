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
      : storage_{dbAdapter_}, sessionMgr_{bus_}, networkServer_{ErrorBus{bus_.systemBus}},
        authenticator_{bus_.systemBus, dbAdapter_}, coordinator_{bus_, storage_.marketData()},
        consoleRdr_{bus_.systemBus}, priceFeed_{bus_, dbAdapter_}, streamAdapter_{bus_} {
    // System bus subscriptions
    bus_.subscribe<ServerEvent>([this](CRef<ServerEvent> event) {
      switch (event.state) {
      case ServerState::Operational:
        // start the network server only after internal components are fully operational
        LOG_INFO_SYSTEM("Server is ready");
        networkServer_.start();
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

    sessionMgr_.setDrainHook(networkServer_.getHook());
  }

  void start() {
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
};

} // namespace hft::server

#endif // HFT_SERVER_SERVERCONTROLCENTER_HPP
