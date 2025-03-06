/**
 * @author Vladimir Pavliv
 * @date 2025-02-13
 */

#ifndef HFT_SERVER_SERVER_HPP
#define HFT_SERVER_SERVER_HPP

#include <format>
#include <memory>
#include <unordered_map>

#include "boost_types.hpp"
#include "comparators.hpp"
#include "config/config.hpp"
#include "control_center.hpp"
#include "engine.hpp"
#include "event_bus.hpp"
#include "market_types.hpp"
#include "network/async_socket.hpp"
#include "network_types.hpp"
#include "server_command.hpp"
#include "template_types.hpp"
#include "types.hpp"
#include "utils/template_utils.hpp"
#include "utils/utils.hpp"

namespace hft::server {

class Server {
  using ServerTcpSocket = AsyncSocket<TcpSocket, Order>;
  using ServerUdpSocket = AsyncSocket<UdpSocket, TickerPrice>;
  using ServerControlCenter = ControlCenter<ServerCommand>;

  struct Session {
    ServerTcpSocket::UPtr ingress;
    ServerTcpSocket::UPtr egress;
  };

public:
  Server()
      : ingressAcceptor_{ioCtx_}, egressAcceptor_{ioCtx_},
        pricesSocket_{utils::createUdpSocket(ioCtx_),
                      UdpEndpoint{Ip::address_v4::broadcast(), Config::cfg.portUdp}},
        engine_{ioCtx_}, controlCenter_{ioCtx_} {
    if (Config::cfg.coreIds.size() == 0 || Config::cfg.coreIds.size() > 10) {
      throw std::runtime_error("Invalid cores configuration");
    }
    utils::unblockConsole();

    EventBus::bus().subscribe<SocketStatusEvent>([this](Span<SocketStatusEvent> events) {
      auto &event = events.front();
      switch (event.status) {
      case SocketStatus::Disconnected:
        Logger::monitorLogger->info("{} disconnected", event.traderId);
        sessions_.erase(event.traderId);
        break;
      default:
        break;
      }
    });

    EventBus::bus().subscribe<OrderStatus>([this](Span<OrderStatus> events) {
      TraderIdCmp<OrderStatus> cmp;
      auto [subSpan, leftover] = utils::frontSubspan(events, cmp);
      while (!subSpan.empty()) {
        sessions_[subSpan.front().traderId].egress->asyncWrite(subSpan);
        std::tie(subSpan, leftover) = utils::frontSubspan(leftover, cmp);
      }
    });

    startIngress();
    startEgress();

    EventBus::bus().subscribe<ServerCommand>([this](Span<ServerCommand> commands) {
      switch (commands.front()) {
      case ServerCommand::Shutdown:
        stop();
        break;
      default:
        break;
      }
    });

    controlCenter_.addCommand("q", ServerCommand::Shutdown);
    controlCenter_.addCommand("p+", ServerCommand::PriceFeedStart);
    controlCenter_.addCommand("p-", ServerCommand::PriceFeedStop);
  }
  ~Server() { stop(); }

  void start() {
    utils::setTheadRealTime();
    controlCenter_.start();
    engine_.start();
    ioCtx_.run();
  }

  void stop() {
    controlCenter_.stop();
    ingressAcceptor_.close();
    egressAcceptor_.close();
    ioCtx_.stop();
    engine_.stop();
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
      Logger::monitorLogger->info("{} connected ingress", traderId);
      sessions_[traderId].ingress = std::make_unique<ServerTcpSocket>(std::move(socket), traderId);
      sessions_[traderId].ingress->asyncRead();
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
      Logger::monitorLogger->info("{} connected egress", traderId);
      sessions_[traderId].egress = std::make_unique<ServerTcpSocket>(std::move(socket), traderId);
      acceptEgress();
    });
  }

private:
  IoContext ioCtx_;

  TcpAcceptor ingressAcceptor_;
  TcpAcceptor egressAcceptor_;
  ServerUdpSocket pricesSocket_;

  Engine engine_;
  ServerControlCenter controlCenter_;

  std::unordered_map<TraderId, Session> sessions_;
};

} // namespace hft::server

#endif // HFT_SERVER_SERVER_HPP