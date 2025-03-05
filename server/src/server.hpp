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
#include "engine.hpp"
#include "event_bus.hpp"
#include "market_types.hpp"
#include "network/async_socket.hpp"
#include "network_types.hpp"
#include "template_types.hpp"
#include "types.hpp"
#include "utils/template_utils.hpp"
#include "utils/utils.hpp"

namespace hft::server {

class Server {
  using ServerTcpSocket = AsyncSocket<TcpSocket, Order>;
  using ServerUdpSocket = AsyncSocket<UdpSocket, TickerPrice>;

  struct Session {
    ServerTcpSocket::UPtr ingress;
    ServerTcpSocket::UPtr egress;
  };

public:
  Server()
      : mIngressAcceptor{mCtx}, mEgressAcceptor{mCtx},
        mPricesSocket{utils::createUdpSocket(mCtx),
                      UdpEndpoint{Ip::address_v4::broadcast(), Config::cfg.portUdp}},
        mEngine{mCtx}, mInputTimer{mCtx} {
    if (Config::cfg.coreIds.size() == 0 || Config::cfg.coreIds.size() > 10) {
      throw std::runtime_error("Invalid cores configuration");
    }
    utils::unblockConsole();

    EventBus::bus().subscribe<SocketStatusEvent>([this](Span<SocketStatusEvent> events) {
      auto &event = events.front();
      switch (event.status) {
      case SocketStatus::Disconnected:
        Logger::monitorLogger->info("{} disconnected", event.traderId);
        mSessions.erase(event.traderId);
        break;
      default:
        break;
      }
    });

    EventBus::bus().subscribe<OrderStatus>([this](Span<OrderStatus> statuses) {
      TraderIdCmp<OrderStatus> cmp;
      auto [subSpan, leftover] = utils::frontSubspan(statuses, cmp);
      while (!subSpan.empty()) {
        mSessions[subSpan.front().traderId].egress->asyncWrite(subSpan);
        std::tie(subSpan, leftover) = utils::frontSubspan(leftover, cmp);
      }
    });

    startIngress();
    startEgress();
    scheduleInputTimer();
  }
  ~Server() { stop(); }

  void start() {
    utils::setTheadRealTime();
    mCtx.run();
  }

  void stop() {
    mIngressAcceptor.close();
    mEgressAcceptor.close();
    mCtx.stop();
    mEngine.stop();
  }

private:
  void startIngress() {
    TcpEndpoint endpoint(Tcp::v4(), Config::cfg.portTcpIn);
    mIngressAcceptor.open(endpoint.protocol());
    mIngressAcceptor.set_option(boost::asio::socket_base::reuse_address(true));
    mIngressAcceptor.bind(endpoint);
    mIngressAcceptor.listen();
    acceptIngress();
  }

  void acceptIngress() {
    mIngressAcceptor.async_accept([this](BoostErrorRef ec, TcpSocket socket) {
      if (ec) {
        spdlog::error("Failed to accept connection {}", ec.message());
        return;
      }
      socket.set_option(TcpSocket::protocol_type::no_delay(true));
      auto traderId = utils::getTraderId(socket);
      Logger::monitorLogger->info("{} connected ingress", traderId);
      mSessions[traderId].ingress = std::make_unique<ServerTcpSocket>(std::move(socket), traderId);
      mSessions[traderId].ingress->asyncRead();
      acceptIngress();
    });
  }

  void startEgress() {
    TcpEndpoint endpoint(Tcp::v4(), Config::cfg.portTcpOut);
    mEgressAcceptor.open(endpoint.protocol());
    mEgressAcceptor.set_option(boost::asio::socket_base::reuse_address(true));
    mEgressAcceptor.bind(endpoint);
    mEgressAcceptor.listen();
    acceptEgress();
  }

  void acceptEgress() {
    mEgressAcceptor.async_accept([this](BoostErrorRef ec, TcpSocket socket) {
      if (ec) {
        spdlog::error("Failed to accept connection: {}", ec.message());
        return;
      }
      socket.set_option(TcpSocket::protocol_type::no_delay(true));
      auto traderId = utils::getTraderId(socket);
      Logger::monitorLogger->info("{} connected egress", traderId);
      mSessions[traderId].egress = std::make_unique<ServerTcpSocket>(std::move(socket), traderId);
      acceptEgress();
    });
  }

  void scheduleInputTimer() {
    mInputTimer.expires_after(Milliseconds(200));
    mInputTimer.async_wait([this](BoostErrorRef ec) {
      if (ec) {
        return;
      }
      checkInput();
      scheduleInputTimer();
    });
  }

  void checkInput() {
    auto cmd = utils::getConsoleInput();
    if (cmd.empty()) {
      return;
    } else if (cmd == "q") {
      stop();
    } else if (cmd == "p+") {
      //
      Logger::monitorLogger->info("Price feed start");
    } else if (cmd == "p-") {
      //
      Logger::monitorLogger->info("Price feed stop");
    }
  }

private:
  IoContext mCtx;

  TcpAcceptor mIngressAcceptor;
  TcpAcceptor mEgressAcceptor;
  ServerUdpSocket mPricesSocket;

  Engine mEngine;
  SteadyTimer mInputTimer;

  std::unordered_map<size_t, Session> mSessions;
};

} // namespace hft::server

#endif // HFT_SERVER_SERVER_HPP