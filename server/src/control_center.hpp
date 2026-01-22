/**
 * @author Vladimir Pavliv
 * @date 2025-03-07
 */

#ifndef HFT_SERVER_CONTROLCENTER_HPP
#define HFT_SERVER_CONTROLCENTER_HPP

#include <boost/asio/signal_set.hpp>
#include <stop_token>

#include "commands/command.hpp"
#include "commands/command_parser.hpp"
#include "config/server_config.hpp"
#include "domain_types.hpp"
#include "events.hpp"
#include "execution/coordinator.hpp"
#include "gateway/order_gateway.hpp"
#include "price_feed.hpp"
#include "session/authenticator.hpp"
#include "storage/storage.hpp"
#include "traits.hpp"
#include "transport/channel.hpp"
#include "transport/shm/shm_server.hpp"
#include "utils/console_reader.hpp"
#include "utils/id_utils.hpp"

namespace hft::server {

/**
 * @brief cc
 * Order event flow ('=>': MarketBus routing, '->': manual routing)
 * [network thread]
 * 1. IpcServer
 *    -> Order read from the network
 *    <= ServerOrder supplied with client id
 * 2. OrderGateway
 *    => ServerOrder, allocates id, creates order record
 *    <= InternalOrderEvent with stripped down metadata
 * 3. Coordinator
 *    => InternalOrderEvent
 *    <- (thread hop) manual dispatch to a proper worker via worker LfqRunner
 * [worker thread]
 * 4. Worker
 *    -> manually dispatched from Coordinator
 *    <= (thread hop) InternalOrderStatus via gateway LfqRunner
 * [gateway thread]
 * 5. OrderGateway
 *    => InternalOrderStatus, update record with local OB id, cleanup if Rejected
 *    <= ServerOrderStatus supplied with client id
 * 6. SessionManager
 *    => ServerOrderStatus
 *    <- manually writes to a proper channel based on client id
 */
class ControlCenter {
public:
  explicit ControlCenter(ServerConfig &&config)
      : config_{std::move(config)}, bus_{config_.data}, ctx_{bus_, config_, stopSrc_.get_token()},
        dbAdapter_{config_.data}, storage_{config_, dbAdapter_}, sessionMgr_{ctx_},
        ipcServer_{ctx_}, authenticator_{ctx_, dbAdapter_},
        coordinator_{ctx_, storage_.marketData()}, gateway_{ctx_},
        consoleReader_{ctx_.bus.systemBus}, priceFeed_{ctx_, dbAdapter_},
        signals_{bus_.systemIoCtx(), SIGINT, SIGTERM} {

    using T = ControlCenter;
    // System bus subscriptions
    bus_.subscribe<ServerEvent>(CRefHandler<ServerEvent>::template bind<T, &T::post>(this));
    bus_.subscribe<TickerPrice>(CRefHandler<TickerPrice>::template bind<T, &T::post>(this));
    bus_.subscribe<InternalError>(CRefHandler<InternalError>::template bind<T, &T::post>(this));

    // network callbacks
    ipcServer_.setUpstreamClb(
        [this](StreamTransport &&transport) { sessionMgr_.acceptUpstream(std::move(transport)); });
    ipcServer_.setDownstreamClb([this](StreamTransport &&transport) {
      sessionMgr_.acceptDownstream(std::move(transport));
    });
    ipcServer_.setDatagramClb([this](DatagramTransport &&transport) {

    });

    bus_.systemBus.subscribe(Command::Shutdown, Callback::template bind<T, &T::stop>(this));

    signals_.async_wait([&](BoostErrorCode code, int) {
      LOG_INFO_SYSTEM("Signal received {}, stopping...", code.message());
      stop();
    });
  }

  ~ControlCenter() { LOG_DEBUG_SYSTEM("~ControlCenter"); }

  void start() {
    if (storage_.marketData().empty()) {
      throw std::runtime_error("No ticker data loaded from db");
    }
    greetings();
    try {
      gateway_.start();
      coordinator_.start();
      bus_.run();
    } catch (const std::exception &e) {
      LOG_ERROR_SYSTEM("Exception in CC::start {}", e.what());
      stop();
    }
  }

  void stop() {
    try {
      stopSrc_.request_stop();

      ipcServer_.stop();
      coordinator_.stop();
      gateway_.stop();
      sessionMgr_.close();
      bus_.stop();

      LOG_INFO_SYSTEM("stonk");
    } catch (const std::exception &e) {
      LOG_ERROR_SYSTEM("Exception in CC::stop {}", e.what());
    }
  }

private:
  void post(CRef<ServerEvent> event) {
    switch (event.state) {
    case ServerState::Operational:
      // start the network server only after internal components are fully operational
      ipcServer_.start();
      LOG_INFO_SYSTEM("Server is ready");
      break;
    default:
      break;
    }
  }

  void post(CRef<TickerPrice>) {}

  void post(CRef<InternalError> event) {
    LOG_ERROR_SYSTEM("Internal error: {} {}", event.what, toString(event.code));
    stop();
  }

  void greetings() {
    LOG_INFO_SYSTEM("Server go stonks");
    LOG_INFO_SYSTEM("Configuration:");
    consoleReader_.printCommands();
    LOG_INFO_SYSTEM("Tickers loaded: {}", storage_.marketData().size());
  }

private:
  const ServerConfig config_;
  std::stop_source stopSrc_;

  ServerBus bus_;
  Context ctx_;

  DbAdapter dbAdapter_;
  Storage storage_;

  IpcServer ipcServer_;
  SessionManager sessionMgr_;
  Authenticator authenticator_;
  Coordinator coordinator_;
  OrderGateway gateway_;
  ServerConsoleReader consoleReader_;
  PriceFeed priceFeed_;

  boost::asio::signal_set signals_;
};

} // namespace hft::server

#endif // HFT_SERVER_CONTROLCENTER_HPP
