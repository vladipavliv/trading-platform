/**
 * @author Vladimir Pavliv
 * @date 2025-02-13
 */

#ifndef HFT_SERVER_NETWORKSERVER_HPP
#define HFT_SERVER_NETWORKSERVER_HPP

#include <format>
#include <memory>
#include <thread>
#include <unordered_map>

#include "boost_types.hpp"
#include "bus/bus.hpp"
#include "comparators.hpp"
#include "config/config.hpp"
#include "market_types.hpp"
#include "network/async_socket.hpp"
#include "network_types.hpp"
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
  using ServerTcpSocket = AsyncSocket<TcpSocket, Order>;
  using ServerUdpSocket = AsyncSocket<UdpSocket, TickerPrice>;

  struct Session {
    ServerTcpSocket::UPtr ingress;
    ServerTcpSocket::UPtr egress;
  };

public:
  NetworkServer(Bus &bus)
      : bus_{bus}, guard_{MakeGuard(ioCtx_.get_executor())}, ingressAcceptor_{ioCtx_},
        egressAcceptor_{ioCtx_}, pricesSocket_{createPricesSocket()} {
    utils::unblockConsole();

    // System bus subscriptions
    bus_.systemBus.subscribe<SocketStatusEvent>([this](CRef<SocketStatusEvent> event) {
      switch (event.status) {
      case SocketStatus::Disconnected:
        Logger::monitorLogger->info("{} disconnected", event.traderId);
        sessions_.erase(event.traderId);
        break;
      case SocketStatus::Connected:
        if (isConnected(event.traderId)) {
          // TODO(self) Allow traffic only on fully connected sessions
          Logger::monitorLogger->info("{} connected", event.traderId);
        }
        break;
      default:
        break;
      }
    });

    // Market messages
    bus_.marketBus.setHandler<OrderStatus>([this](Span<OrderStatus> events) {
      TraderIdCmp<OrderStatus> cmp;
      auto [subSpan, leftover] = utils::frontSubspan(events, cmp);
      while (!subSpan.empty()) {
        sessions_[subSpan.front().traderId].egress->asyncWrite(subSpan);
        std::tie(subSpan, leftover) = utils::frontSubspan(leftover, cmp);
      }
    });
    bus_.marketBus.setHandler<TickerPrice>(
        [this](Span<TickerPrice> events) { pricesSocket_.asyncWrite(events); });
  }
  ~NetworkServer() {
    stop();
    for (auto &thread : workerThreads_) {
      if (thread.joinable()) {
        thread.join();
      }
    }
  }

  void start() {
    const auto cores = Config::cfg.coresNetwork.size();
    Logger::monitorLogger->info("Starting network server on {} threads", cores);
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
    ingressAcceptor_.async_accept([this](BoostErrorRef ec, TcpSocket socket) {
      if (ec) {
        spdlog::error("Failed to accept connection {}", ec.message());
        return;
      }
      socket.set_option(TcpSocket::protocol_type::no_delay(true));
      auto traderId = utils::getTraderId(socket);
      sessions_[traderId].ingress =
          std::make_unique<ServerTcpSocket>(std::move(socket), bus_, traderId);
      sessions_[traderId].ingress->asyncRead();
      if (isConnected(traderId)) {
        Logger::monitorLogger->info("{} connected", traderId);
      }
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
    egressAcceptor_.async_accept([this](BoostErrorRef ec, TcpSocket socket) {
      if (ec) {
        spdlog::error("Failed to accept connection: {}", ec.message());
        return;
      }
      socket.set_option(TcpSocket::protocol_type::no_delay(true));
      auto traderId = utils::getTraderId(socket);
      auto &session = sessions_[traderId];
      session.egress = std::make_unique<ServerTcpSocket>(std::move(socket), bus_, traderId);
      if (isConnected(traderId)) {
        Logger::monitorLogger->info("{} connected", traderId);
      }
      acceptEgress();
    });
  }

  bool isConnected(TraderId traderId) {
    auto &session = sessions_[traderId];
    return session.egress != nullptr && session.ingress != nullptr &&
           session.egress->status() == SocketStatus::Connected &&
           session.ingress->status() == SocketStatus::Connected;
  }

  ServerUdpSocket createPricesSocket() {
    return ServerUdpSocket{utils::createUdpSocket(ioCtx_), bus_,
                           UdpEndpoint{Ip::address_v4::broadcast(), Config::cfg.portUdp}};
  }

private:
  IoContext ioCtx_;
  ContextGuard guard_;

  Bus &bus_;

  TcpAcceptor ingressAcceptor_;
  TcpAcceptor egressAcceptor_;
  ServerUdpSocket pricesSocket_;

  std::unordered_map<TraderId, Session> sessions_;
  std::vector<std::thread> workerThreads_;
};

} // namespace hft::server

#endif // HFT_SERVER_NETWORKSERVER_HPP
