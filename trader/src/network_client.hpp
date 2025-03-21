/**
 * @author Vladimir Pavliv
 * @date 2025-02-13
 */

#ifndef HFT_SERVER_NETWORKCLIENT_HPP
#define HFT_SERVER_NETWORKCLIENT_HPP

#include <format>
#include <memory>
#include <spdlog/spdlog.h>
#include <thread>
#include <vector>

#include "boost_types.hpp"
#include "bus/bus.hpp"
#include "comparators.hpp"
#include "config/config.hpp"
#include "logger.hpp"
#include "market_types.hpp"
#include "network/async_socket.hpp"
#include "network_types.hpp"
#include "template_types.hpp"
#include "trader_command.hpp"
#include "trader_event.hpp"
#include "types.hpp"
#include "utils/utils.hpp"

namespace hft::trader {

/**
 * @brief Manages connections to the server
 * Runs a separate io_context on a number of threads, connects/reconnects
 */
class NetworkClient {
  using TraderTcpSocket = AsyncSocket<TcpSocket, OrderStatus>;
  using TraderUdpSocket = AsyncSocket<UdpSocket, TickerPrice>;

public:
  NetworkClient(Bus &bus)
      : ioCtxGuard_{MakeGuard(ioCtx_.get_executor())}, bus_{bus},
        ingressSocket_{createIngressSocket()}, egressSocket_{createEgressSocket()},
        pricesSocket_{createPricesSocket()}, connectionTimer_{ioCtx_},
        monitorRate_{Config::cfg.monitorRate} {
    utils::unblockConsole();

    bus_.marketBus.setHandler<Order>(
        [this](Span<Order> orders) { egressSocket_.asyncWrite(orders); });

    bus_.systemBus.subscribe<SocketStatusEvent>(
        [this](CRef<SocketStatusEvent> event) { onSocketStatus(event); });
  }

  ~NetworkClient() {
    stop();
    for (auto &thread : workerThreads_) {
      if (thread.joinable()) {
        thread.join();
      }
    }
  }

  void start() {
    const auto cores = Config::cfg.coresNetwork.size();
    Logger::monitorLogger->info("Starting network client on {} threads", cores);
    workerThreads_.reserve(cores);
    for (int i = 0; i < cores; ++i) {
      workerThreads_.emplace_back([this, i]() {
        try {
          utils::setTheadRealTime();
          utils::pinThreadToCore(Config::cfg.coresNetwork[i]);
          ioCtx_.run();
        } catch (const std::exception &e) {
          Logger::monitorLogger->error("Exception in network thread {}", e.what());
        }
      });
    }
    ioCtx_.post([this]() {
      Logger::monitorLogger->info("Connecting to the server");
      scheduleConnectionTimer();
      ingressSocket_.asyncConnect();
      egressSocket_.asyncConnect();
      pricesSocket_.asyncConnect();
    });
  }

  void stop() { ioCtx_.stop(); }

  bool connected() const { return connected_.load(); }

private:
  void onSocketStatus(SocketStatusEvent event) {
    switch (event.status) {
    case SocketStatus::Connected:
      if (ingressSocket_.status() == SocketStatus::Connected &&
          egressSocket_.status() == SocketStatus::Connected) {
        if (!connected_) {
          connected_ = true;
          ingressSocket_.asyncRead();
          bus_.systemBus.publish(TraderEvent::ConnectedToTheServer);
        }
      }
      break;
    case SocketStatus::Disconnected:
      if (connected_) {
        connected_ = false;
        bus_.systemBus.publish(TraderEvent::DisconnectedFromTheServer);
      }
      break;
    default:
      break;
    }
  }

  void scheduleConnectionTimer() {
    connectionTimer_.expires_after(monitorRate_);
    connectionTimer_.async_wait([this](BoostErrorRef ec) {
      if (ec) {
        return;
      }
      const bool ingressOn = ingressSocket_.status() == SocketStatus::Connected;
      const bool egressOn = egressSocket_.status() == SocketStatus::Connected;
      if (!ingressOn || !egressOn) {
        Logger::monitorLogger->critical("Server is down, reconnecting...");
        if (!ingressOn) {
          ingressSocket_.reconnect();
        }
        // Egress socket won't passively notify about disconnect
        egressSocket_.reconnect();
      }
      scheduleConnectionTimer();
    });
  }

  /**
   * @brief Ctor readability helpers
   */
  TraderTcpSocket createIngressSocket() {
    return TraderTcpSocket{TcpSocket{ioCtx_}, bus_,
                           TcpEndpoint{Ip::make_address(Config::cfg.url), Config::cfg.portTcpOut}};
  }

  TraderTcpSocket createEgressSocket() {
    return TraderTcpSocket{TcpSocket{ioCtx_}, bus_,
                           TcpEndpoint{Ip::make_address(Config::cfg.url), Config::cfg.portTcpIn}};
  }

  TraderUdpSocket createPricesSocket() {
    return TraderUdpSocket{utils::createUdpSocket(ioCtx_, false, Config::cfg.portUdp), bus_,
                           UdpEndpoint(Udp::v4(), Config::cfg.portUdp)};
  }

private:
  IoContext ioCtx_;
  ContextGuard ioCtxGuard_;

  Bus &bus_;

  TraderTcpSocket ingressSocket_;
  TraderTcpSocket egressSocket_;
  TraderUdpSocket pricesSocket_;

  SteadyTimer connectionTimer_;
  Seconds monitorRate_;

  std::atomic_bool connected_{false};
  std::vector<std::thread> workerThreads_;
};

} // namespace hft::trader

#endif // HFT_SERVER_NETWORKCLIENT_HPP
