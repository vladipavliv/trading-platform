/**
 * @author Vladimir Pavliv
 * @date 2025-02-13
 */

#ifndef HFT_SERVER_NETWORKSERVER_HPP
#define HFT_SERVER_NETWORKSERVER_HPP

#include <algorithm>
#include <format>
#include <memory>

#include "boost_types.hpp"
#include "broadcast_channel.hpp"
#include "commands/server_command.hpp"
#include "config/server_config.hpp"
#include "domain_types.hpp"
#include "internal_error.hpp"
#include "network/channels/udp_channel.hpp"
#include "server_events.hpp"
#include "server_types.hpp"
#include "session_channel.hpp"
#include "types.hpp"
#include "utils/utils.hpp"

namespace hft::server {

/**
 * @brief Runs network io_context. Starts tcp acceptors and broadcast service
 * Redirects accepted tcp sockets to the gateway
 */
template <typename Listener>
class NetworkServer {
public:
  using SessionChannelType = SessionChannel;
  using BroadcastChannelType = BroadcastChannel;
  using ListenerType = Listener;

  NetworkServer(ServerBus &bus, Listener &listener)
      : guard_{MakeGuard(ioCtx_.get_executor())}, bus_{bus}, listener_{listener},
        upstreamAcceptor_{ioCtx_}, downstreamAcceptor_{ioCtx_} {
    ioCtx_.post([this]() {
      startUpstream();
      startDownstream();
      const auto id = utils::generateConnectionId();
      listener_.acceptBroadcast(std::make_shared<BroadcastChannelType>(id, ioCtx_, bus_));
      LOG_INFO_SYSTEM("Network server started");
    });
  }

  ~NetworkServer() { stop(); }

  inline void pollOne() { ioCtx_.poll_one(); }

  void stop() { ioCtx_.stop(); }

private:
  void startUpstream() {
    const TcpEndpoint endpoint(Tcp::v4(), ServerConfig::cfg.portTcpUp);
    upstreamAcceptor_.open(endpoint.protocol());
    upstreamAcceptor_.set_option(boost::asio::socket_base::reuse_address(true));
    upstreamAcceptor_.bind(endpoint);
    upstreamAcceptor_.listen();
    acceptUpstream();
  }

  void acceptUpstream() {
    upstreamAcceptor_.async_accept([this](BoostErrorCode ec, TcpSocket socket) {
      if (ec) {
        LOG_ERROR("Failed to accept connection {}", ec.message());
        return;
      }
      socket.set_option(TcpSocket::protocol_type::no_delay(true));

      const auto id = utils::generateConnectionId();
      auto channel = std::make_shared<SessionChannel>(id, std::move(socket), bus_);
      listener_.acceptUpstream(std::move(channel));
      acceptUpstream();
    });
  }

  void startDownstream() {
    const TcpEndpoint endpoint(Tcp::v4(), ServerConfig::cfg.portTcpDown);
    downstreamAcceptor_.open(endpoint.protocol());
    downstreamAcceptor_.set_option(boost::asio::socket_base::reuse_address(true));
    downstreamAcceptor_.bind(endpoint);
    downstreamAcceptor_.listen();
    acceptDownstream();
  }

  void acceptDownstream() {
    downstreamAcceptor_.async_accept([this](BoostErrorCode ec, TcpSocket socket) {
      if (ec) {
        LOG_ERROR("Failed to accept connection: {}", ec.message());
        return;
      }
      socket.set_option(TcpSocket::protocol_type::no_delay(true));

      const auto id = utils::generateConnectionId();
      auto channel = std::make_shared<SessionChannel>(id, std::move(socket), bus_);
      listener_.acceptDownstream(std::move(channel));
      acceptDownstream();
    });
  }

private:
  IoCtx ioCtx_;
  ContextGuard guard_;

  ServerBus &bus_;
  Listener &listener_;

  TcpAcceptor upstreamAcceptor_;
  TcpAcceptor downstreamAcceptor_;
};

} // namespace hft::server

#endif // HFT_SERVER_NETWORKSERVER_HPP
