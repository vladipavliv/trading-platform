/**
 * @author Vladimir Pavliv
 * @date 2025-02-13
 */

#ifndef HFT_TRADER_NETWORKCLIENT_HPP
#define HFT_TRADER_NETWORKCLIENT_HPP

#include <format>
#include <memory>
#include <spdlog/spdlog.h>
#include <thread>
#include <vector>

#include "boost_types.hpp"
#include "bus/bus.hpp"
#include "bus/subscription_holder.hpp"
#include "comparators.hpp"
#include "config/config.hpp"
#include "io_ctx.hpp"
#include "logger.hpp"
#include "market_types.hpp"
#include "network/socket_status.hpp"
#include "network/transport/tcp_transport.hpp"
#include "network/transport/udp_transport.hpp"
#include "network_types.hpp"
#include "serialization/flat_buffers/fb_serializer.hpp"
#include "template_types.hpp"
#include "trader_command.hpp"
#include "trader_events.hpp"
#include "trader_gateway.hpp"
#include "types.hpp"
#include "utils/utils.hpp"

namespace hft::trader {

/**
 * @brief Manages connections to the server
 * Runs a separate io_context on a number of threads, connects/reconnects
 */
class NetworkClient {
  using Serializer = serialization::FlatBuffersSerializer;
  using Framer = SizeFramer<Serializer>;
  using TraderTcpTransport = TcpTransport<TraderGateway, Framer>;
  using TraderUdpTransport = UdpTransport<Framer>;

public:
  NetworkClient(Bus &bus)
      : bus_{bus}, ingressTransport_{createIngressTransport()},
        egressTransport_{createEgressTransport()}, pricesTransport_{createPricesTransport()},
        subs_{id_, bus_.systemBus}, connectionTimer_{ioCtx_.ctx},
        monitorRate_{Config::cfg.monitorRate} {
    bus_.marketBus.setHandler<Order>(
        [this](Span<Order> orders) { egressTransport_.write(orders); });

    bus_.systemBus.subscribe<LoginRequestEvent>(
        id_, subs_.add<CRefHandler<LoginRequestEvent>>(
                 [this](CRef<LoginRequestEvent> event) { onLoginRequest(event); }));

    bus_.systemBus.subscribe<LoginResponseEvent>(
        id_, subs_.add<CRefHandler<LoginResponseEvent>>(
                 [this](CRef<LoginResponseEvent> event) { onLoginResponse(event); }));
  }

  ~NetworkClient() { stop(); }

  void start() {
    const auto cores = Config::cfg.coresNetwork.size();
    Logger::monitorLogger->info("Starting network client on {} threads", cores);
    workerThreads_.reserve(cores);
    for (int i = 0; i < cores; ++i) {
      workerThreads_.emplace_back([this, i]() {
        try {
          auto coreId = Config::cfg.coresNetwork[i];
          utils::setTheadRealTime(coreId);
          utils::pinThreadToCore(coreId);
          ioCtx_.run();
        } catch (const std::exception &e) {
          Logger::monitorLogger->error("Exception in network thread {}", e.what());
        }
      });
    }
    ioCtx_.post([this]() {
      Logger::monitorLogger->info("Connecting to the server");
      scheduleConnectionTimer();
      ingressTransport_.connect();
      egressTransport_.connect();
      pricesTransport_.read();
    });
  }

  void stop() { ioCtx_.stop(); }

  inline bool connected() const { return connected_.load(); }

private:
  void onLoginRequest(CRef<LoginRequestEvent> event) {
    LoginRequest request = event.request;
    if (event.connectionId == ingressTransport_.id()) {
      ingressTransport_.write(Span<LoginRequest>(&request, 1));
    } else if (event.connectionId == egressTransport_.id()) {
      egressTransport_.write(Span<LoginRequest>(&request, 1));
    } else {
      Logger::monitorLogger->error("Invalid connection id {}", event.connectionId);
    }
  }

  void onLoginResponse(CRef<LoginResponseEvent> event) {
    if (!event.response.success) {
      Logger::monitorLogger->error("Login failed");
      return;
    } else {
      Logger::monitorLogger->error("Logged in {} {}", event.connectionId, event.response.token);
    }
  }

  void scheduleConnectionTimer() {
    connectionTimer_.expires_after(monitorRate_);
    connectionTimer_.async_wait([this](CRef<BoostError> ec) {
      if (ec) {
        return;
      }
      const bool ingressOn = ingressTransport_.status() == SocketStatus::Connected;
      const bool egressOn = egressTransport_.status() == SocketStatus::Connected;
      if (!ingressOn || !egressOn) {
        Logger::monitorLogger->critical("Server is down, reconnecting...");
        if (!ingressOn) {
          ingressTransport_.reconnect();
        }
        // Egress socket won't passively notify about disconnect, force reconnect
        egressTransport_.reconnect();
        scheduleConnectionTimer();
      }
    });
  }

  TraderTcpTransport createIngressTransport() {
    return TraderTcpTransport{
        TcpSocket{ioCtx_.ctx},
        TcpEndpoint{Ip::make_address(Config::cfg.url), Config::cfg.portTcpOut}, TraderGateway{bus_},
        TcpConnectionType::Ingress};
  }

  TraderTcpTransport createEgressTransport() {
    return TraderTcpTransport{TcpSocket{ioCtx_.ctx},
                              TcpEndpoint{Ip::make_address(Config::cfg.url), Config::cfg.portTcpIn},
                              TraderGateway{bus_}, TcpConnectionType::Egress};
  }

  TraderUdpTransport createPricesTransport() {
    return TraderUdpTransport{utils::createUdpSocket(ioCtx_.ctx, false, Config::cfg.portUdp),
                              UdpEndpoint(Udp::v4(), Config::cfg.portUdp), bus_};
  }

private:
  const ObjectId id_{utils::getId()};

  IoCtx ioCtx_;
  Bus &bus_;

  TraderTcpTransport ingressTransport_;
  TraderTcpTransport egressTransport_;
  TraderUdpTransport pricesTransport_;

  SubscriptionHolder subs_;

  SteadyTimer connectionTimer_;
  Seconds monitorRate_;

  std::atomic_bool connected_{false};
  std::vector<Thread> workerThreads_;
};

} // namespace hft::trader

#endif // HFT_TRADER_NETWORKCLIENT_HPP
