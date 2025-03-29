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

#include "boost_types.hpp"
#include "bus/bus.hpp"
#include "comparators.hpp"
#include "config/config.hpp"
#include "market_types.hpp"
#include "network/connection_status.hpp"
#include "network/size_framer.hpp"
#include "network/transport/tcp_transport.hpp"
#include "network/transport/udp_transport.hpp"
#include "network_types.hpp"
#include "serialization/flat_buffers/fb_serializer.hpp"
#include "server_command.hpp"
#include "template_types.hpp"
#include "types.hpp"
#include "utils/template_utils.hpp"
#include "utils/utils.hpp"

namespace hft::server {

/**
 * @brief Accepts trader connections and manages sessions
 * @details Runs in a separate io_context on a number of threads
 */
class NetworkServer {
  using Serializer = serialization::FlatBuffersSerializer;
  using Framer = SizeFramer<Serializer>;
  using ServerTcpTransport = TcpTransport<Framer>;
  using ServerUdpTransport = UdpTransport<Framer>;

  struct Session {
    ServerTcpTransport::UPtr ingress;
    ServerTcpTransport::UPtr egress;
  };

public:
  NetworkServer(Bus &bus)
      : bus_{bus}, guard_{MakeGuard(ioCtx_.get_executor())}, ingressAcceptor_{ioCtx_},
        egressAcceptor_{ioCtx_}, udpTransport_{createUdpTransport()} {
    // System bus subscriptions
    bus_.systemBus.subscribe<TcpConnectionStatus>([this](CRef<TcpConnectionStatus> event) {
      switch (event.status) {
      case ConnectionStatus::Connected:
        if (isConnected(event.traderId)) {
          // TODO(self) Allow traffic only on fully connected sessions
          Logger::monitorLogger->info("{} connected", event.traderId);
        }
        break;
      default:
        Logger::monitorLogger->info("{} disconnected", event.traderId);
        sessions_.erase(event.traderId);
        break;
      }
    });
    bus_.systemBus.subscribe<UdpConnectionStatus>([this](CRef<UdpConnectionStatus> event) {
      if (event.status == ConnectionStatus::Error) {
        Logger::monitorLogger->error("Udp connection error");
        udpTransport_.close();
      }
    });

    // Market messages
    bus_.marketBus.setHandler<OrderStatus>([this](Span<OrderStatus> events) {
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
    });
    bus_.marketBus.setHandler<TickerPrice>(
        [this](Span<TickerPrice> events) { udpTransport_.write(events); });
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
      auto id = utils::getTraderId(socket);
      sessions_[id].ingress =
          std::make_unique<ServerTcpTransport>(std::move(socket), BusWrapper{bus_, id});
      sessions_[id].ingress->read();
      Logger::monitorLogger->info("{} connected ingress", id);
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
      auto id = utils::getTraderId(socket);
      sessions_[id].egress =
          std::make_unique<ServerTcpTransport>(std::move(socket), BusWrapper{bus_, id});
      Logger::monitorLogger->info("{} connected egress", id);
      acceptEgress();
    });
  }

  bool isConnected(TraderId traderId) {
    if (sessions_.count(traderId) == 0) {
      return false;
    }
    auto &session = sessions_[traderId];
    return session.egress != nullptr && session.ingress != nullptr &&
           session.egress->status() == ConnectionStatus::Connected &&
           session.ingress->status() == ConnectionStatus::Connected;
  }

  ServerUdpTransport createUdpTransport() {
    return ServerUdpTransport{utils::createUdpSocket(ioCtx_),
                              UdpEndpoint{Ip::address_v4::broadcast(), Config::cfg.portUdp},
                              BusWrapper(bus_)};
  }

private:
  IoContext ioCtx_;
  ContextGuard guard_;

  Bus &bus_;

  TcpAcceptor ingressAcceptor_;
  TcpAcceptor egressAcceptor_;
  ServerUdpTransport udpTransport_;

  std::unordered_map<TraderId, Session> sessions_;
  std::vector<Thread> workerThreads_;
};

} // namespace hft::server

#endif // HFT_SERVER_NETWORKSERVER_HPP
