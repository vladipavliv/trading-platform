/**
 * @author Vladimir Pavliv
 * @date 2025-03-07
 */

#ifndef HFT_SERVER_CONTROLCENTER_HPP
#define HFT_SERVER_CONTROLCENTER_HPP

#include <boost/asio/signal_set.hpp>

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
 * Order event flow
 * 1. IpcServer (network thread)
 *    consume: Order
 *    produce: ServerOrder supplied with client id
 * 2. OrderGateway (network thread)
 *    consume: ServerOrder, allocates id, creates order record
 *    produce: InternalOrderEvent with stripped down metadata
 * 3. Coordinator (network thread)
 *    cunsume: InternalOrderEvent
 *    produce: (thread hop) manual dispatch to a proper worker via worker LfqRunner
 * 4. Worker (worker thread)
 *    cunsume: manually dispatched from Coordinator
 *    produce: (thread hop) InternalOrderStatus via gateway LfqRunner
 * 5. OrderGateway (gateway thread)
 *    cunsume: InternalOrderStatus, update record with local OB id, cleanup if Rejected
 *    produce: ServerOrderStatus
 * 6. SessionManager (gateway thread)
 *    consume: ServerOrderStatus
 *    produce: manually writes to a proper channel based on client id
 *
 * Threading model:
 * Most important thread cache-wise is workers thread, so trashing its cache is minimized.
 * Order event is popped from lfq, added to the book, status is posted and handled via lfq runner
 * offloading it right away to another thread.
 * Network and gateway threads touch in the gateway. Ensuring they do not touch the same data
 * is done via thread safe id pool, network thread allocates the id, writes the record producing
 * InternalOrderEvent for the worker, and never touches that record, until its released to the pool.
 * Gateway thread takes over that record when status notification comes from the worker,
 * until order gets closed, when it releases id to the pool
 */
class ControlCenter {
public:
  ControlCenter()
      : storage_{dbAdapter_}, sessionMgr_{bus_}, ipcServer_{bus_},
        authenticator_{bus_.systemBus, dbAdapter_}, coordinator_{bus_, storage_.marketData()},
        gateway_{bus_}, consoleReader_{bus_.systemBus}, priceFeed_{bus_, dbAdapter_},
        signals_{bus_.systemIoCtx(), SIGINT, SIGTERM} {

    using SelfT = ControlCenter;
    // System bus subscriptions
    bus_.subscribe<ServerEvent>(CRefHandler<ServerEvent>::template bind<SelfT, &SelfT::post>(this));
    bus_.subscribe<TickerPrice>(CRefHandler<TickerPrice>::template bind<SelfT, &SelfT::post>(this));
    bus_.subscribe<InternalError>(
        CRefHandler<InternalError>::template bind<SelfT, &SelfT::post>(this));

    // network callbacks
    ipcServer_.setUpstreamClb(
        [this](StreamTransport &&transport) { sessionMgr_.acceptUpstream(std::move(transport)); });
    ipcServer_.setDownstreamClb([this](StreamTransport &&transport) {
      sessionMgr_.acceptDownstream(std::move(transport));
    });
    ipcServer_.setDatagramClb([this](DatagramTransport &&transport) {

    });

    bus_.systemBus.subscribe(Command::Shutdown, Callback::template bind<SelfT, &SelfT::stop>(this));

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

    gateway_.start();
    coordinator_.start();
    bus_.run();
  }

  void stop() {
    LOG_INFO_SYSTEM("stonk");

    ipcServer_.stop();
    coordinator_.stop();
    gateway_.stop();
    sessionMgr_.close();
    bus_.stop();
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
    ServerConfig::cfg().log();
    consoleReader_.printCommands();
    LOG_INFO_SYSTEM("Tickers loaded: {}", storage_.marketData().size());
  }

private:
  ServerBus bus_;

  DbAdapter dbAdapter_;
  Storage storage_;

  SessionManager sessionMgr_;
  IpcServer ipcServer_;
  Authenticator authenticator_;
  Coordinator coordinator_;
  OrderGateway gateway_;
  ServerConsoleReader consoleReader_;
  PriceFeed priceFeed_;

  boost::asio::signal_set signals_;
};

} // namespace hft::server

#endif // HFT_SERVER_CONTROLCENTER_HPP
