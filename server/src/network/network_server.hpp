/**
 * @author Vladimir Pavliv
 * @date 2025-02-13
 */

#ifndef HFT_SERVER_NETWORKSERVER_HPP
#define HFT_SERVER_NETWORKSERVER_HPP

#include <algorithm>
#include <format>
#include <memory>
#include <unordered_map>

#include "authenticator.hpp"
#include "boost_types.hpp"
#include "bus/bus.hpp"
#include "bus/subscription_holder.hpp"
#include "comparators.hpp"
#include "config/config.hpp"
#include "io_ctx.hpp"
#include "market_types.hpp"
#include "network/size_framer.hpp"
#include "network/socket_status.hpp"
#include "network/transport/tcp_transport.hpp"
#include "network/transport/udp_transport.hpp"
#include "network_types.hpp"
#include "serialization/flat_buffers/fb_serializer.hpp"
#include "server_command.hpp"
#include "server_events.hpp"
#include "server_gateway.hpp"
#include "template_types.hpp"
#include "types.hpp"
#include "utils/template_utils.hpp"
#include "utils/utils.hpp"

namespace hft::server {

/**
 * @brief Accepts trader connections and manages sessions
 * @details Runs in a separate io_context on a number of threads
 * @details Login process for now is simplified, client must send credentials for both
 * ingress and egress sockets.
 * Until connection is authorized, it is stored in a separate container.
 * When authorization succeeds and TraderId is acquired connection is moved
 * to authorized sessions container
 */
class NetworkServer {
  using Serializer = serialization::FlatBuffersSerializer;
  using Framer = SizeFramer<Serializer>;
  using ServerTcpTransport = TcpTransport<ServerGateway, Framer>;
  using ServerUdpTransport = UdpTransport<Framer>;

  struct Session {
    ServerTcpTransport::UPtr ingress;
    ServerTcpTransport::UPtr egress;
  };

public:
  NetworkServer(Bus &bus)
      : bus_{bus}, ingressAcceptor_{ioCtx_.ctx}, egressAcceptor_{ioCtx_.ctx},
        udpTransport_{createUdpTransport()}, authenticator_{bus_.systemBus},
        subs_{id_, bus_.systemBus} {
    // Market messages
    bus_.marketBus.setHandler<OrderStatus>(
        [this](Span<OrderStatus> events) { onOrderStatus(events); });
    bus_.marketBus.setHandler<TickerPrice>(
        [this](Span<TickerPrice> events) { udpTransport_.write(events); });

    // System bus subscriptions
    bus_.systemBus.subscribe<LoginResponseEvent>(
        id_, subs_.add<CRefHandler<LoginResponseEvent>>(
                 [this](CRef<LoginResponseEvent> event) { onLoginResponse(event); }));

    bus_.systemBus.subscribe<SessionStatusEvent>(
        id_, subs_.add<CRefHandler<SessionStatusEvent>>(
                 [this](CRef<SessionStatusEvent> event) { onSessionStatus(event); }));
  }

  ~NetworkServer() { stop(); }

  void start() {
    const auto cores = Config::cfg.coresNetwork.size();
    Logger::monitorLogger->info("Starting network server on {} threads", cores);
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
      startIngress();
      startEgress();
      Logger::monitorLogger->info("Network server started");
    });
  }

  void stop() {
    ingressAcceptor_.close();
    egressAcceptor_.close();
    ioCtx_.stop();
  }

private:
  void startIngress() {
    TcpEndpoint endpoint(Tcp::v4(), Config::cfg.portTcpIn);
    ingressAcceptor_.open(endpoint.protocol());
    ingressAcceptor_.set_option(boost::asio::socket_base::reuse_address(true));
    ingressAcceptor_.bind(endpoint);
    ingressAcceptor_.listen();
    acceptIngress();
  }

  void acceptIngress() {
    ingressAcceptor_.async_accept([this](CRef<BoostError> ec, TcpSocket socket) {
      if (ec) {
        spdlog::error("Failed to accept connection {}", ec.message());
        return;
      }
      socket.set_option(TcpSocket::protocol_type::no_delay(true));
      auto connection = std::make_unique<ServerTcpTransport>(
          // create new ingress connection and wait for authorization
          std::move(socket), ServerGateway{bus_}, TcpConnectionType::Ingress);
      auto id = connection->id();
      unauthorizedConnections_[id] = std::move(connection);
      unauthorizedConnections_[id]->read();
      spdlog::error("Accepted ingress connection id {}", id);
      acceptIngress();
    });
  }

  void startEgress() {
    TcpEndpoint endpoint(Tcp::v4(), Config::cfg.portTcpOut);
    egressAcceptor_.open(endpoint.protocol());
    egressAcceptor_.set_option(boost::asio::socket_base::reuse_address(true));
    egressAcceptor_.bind(endpoint);
    egressAcceptor_.listen();
    acceptEgress();
  }

  void acceptEgress() {
    egressAcceptor_.async_accept([this](CRef<BoostError> ec, TcpSocket socket) {
      if (ec) {
        spdlog::error("Failed to accept connection: {}", ec.message());
        return;
      }
      socket.set_option(TcpSocket::protocol_type::no_delay(true));
      auto connection = std::make_unique<ServerTcpTransport>(
          // create new egress connection and wait for authorization
          std::move(socket), ServerGateway{bus_}, TcpConnectionType::Egress);
      auto id = connection->id();
      unauthorizedConnections_[id] = std::move(connection);
      unauthorizedConnections_[id]->read();
      spdlog::error("Accepted egress connection id {}", id);
      acceptEgress();
    });
  }

  void onOrderStatus(Span<OrderStatus> events) {
    spdlog::debug("sending {} order statuses", events.size());
    TraderIdCmp<OrderStatus> cmp;
    std::ranges::sort(events, cmp);
    auto [subSpan, leftover] = utils::frontSubspan(events, cmp);
    while (!subSpan.empty()) {
      auto traderId = subSpan.front().traderId;
      if (sessions_.count(traderId) > 0) {
        sessions_[traderId].egress->write(subSpan);
      } else {
        spdlog::debug("trader {} is disconnected", traderId);
      }
      std::tie(subSpan, leftover) = utils::frontSubspan(leftover, cmp);
    }
  }

  void onSessionStatus(CRef<SessionStatusEvent> event) {
    // TODO
    switch (event.status) {
    case SessionStatus::Connected:
      Logger::monitorLogger->info("{} connected", event.traderId);
      break;
    case SessionStatus::Authenticated:
      Logger::monitorLogger->info("{} authenticated", event.traderId);
      break;
    case SessionStatus::Disconnected:
      Logger::monitorLogger->info("{} disconnected", event.traderId);
    case SessionStatus::Error:
    default:
      unauthorizedConnections_.erase(event.connectionId);
      sessions_.erase(event.traderId);
      break;
    }
  }

  void onLoginResponse(CRef<LoginResponseEvent> event) {
    spdlog::info("NetworkServer {}", [&event] { return utils::toString(event); }());
    if (!event.response.success) {
      spdlog::error("Login failed for {}", event.connectionId);
      unauthorizedConnections_.erase(event.connectionId);
      return;
    }
    if (unauthorizedConnections_.count(event.connectionId) == 0 ||
        unauthorizedConnections_[event.connectionId] == nullptr) {
      spdlog::error("No such connection {}", event.connectionId);
      unauthorizedConnections_.erase(event.connectionId);
      return;
    }
    auto connection = std::move(unauthorizedConnections_[event.connectionId]);
    unauthorizedConnections_.erase(event.connectionId);

    LoginResponse response = event.response;
    auto &session = sessions_[event.traderId];

    if (connection->type() == TcpConnectionType::Ingress) {
      if (session.ingress != nullptr) {
        Logger::monitorLogger->error("{} is already logged in ingress", event.traderId);
        return;
      }
      session.ingress = std::move(connection);
      session.ingress->write(Span<LoginResponse>(&response, 1));
      Logger::monitorLogger->info("{} logged in ingress", event.traderId);
    } else {
      if (session.egress != nullptr) {
        Logger::monitorLogger->error("{} is already logged in egress", event.traderId);
        return;
      }
      session.egress = std::move(connection);
      session.egress->write(Span<LoginResponse>(&response, 1));
      Logger::monitorLogger->info("{} logged in egress", event.traderId);
    }
  }

  ServerUdpTransport createUdpTransport() {
    return ServerUdpTransport{utils::createUdpSocket(ioCtx_),
                              UdpEndpoint{Ip::address_v4::broadcast(), Config::cfg.portUdp}, bus_};
  }

private:
  const ObjectId id_{utils::getId()};

  IoCtx ioCtx_;
  Bus &bus_;

  TcpAcceptor ingressAcceptor_;
  TcpAcceptor egressAcceptor_;
  ServerUdpTransport udpTransport_;
  Authenticator authenticator_;

  SubscriptionHolder subs_;

  std::unordered_map<ObjectId, ServerTcpTransport::UPtr> unauthorizedConnections_;
  std::unordered_map<TraderId, Session> sessions_;

  std::vector<Thread> workerThreads_;
};

} // namespace hft::server

#endif // HFT_SERVER_NETWORKSERVER_HPP
