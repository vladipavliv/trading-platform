/**
 * @author Vladimir Pavliv
 * @date 2025-02-13
 */

#ifndef HFT_SERVER_NETWORK_INGRESSSERVER_HPP
#define HFT_SERVER_NETWORK_INGRESSSERVER_HPP

#include <unordered_map>

#include "config/config.hpp"
#include "market_types.hpp"
#include "network_types.hpp"
#include "server_types.hpp"
#include "types.hpp"
#include "utils/utils.hpp"

namespace hft::server::network {

class IngressServer {
public:
  using Socket = ServerSocket<TcpSocket, Order>;

  IngressServer(ServerSink &sink)
      : mSink{sink}, mAcceptor{mSink.ctx()}, mPort{Config::cfg.portTcpIn} {}

  void start() {
    TcpEndpoint endpoint(Tcp::v4(), mPort);
    mAcceptor.open(endpoint.protocol());
    mAcceptor.bind(endpoint);
    mAcceptor.listen();
    acceptConnection();
  }

private:
  void acceptConnection() {
    mAcceptor.set_option(TcpSocket::reuse_address(true));
    mAcceptor.async_accept([this](BoostErrorRef ec, TcpSocket socket) {
      if (ec) {
        spdlog::error("Failed to accept connection {}", ec.message());
        return;
      }
      auto conn = std::make_unique<Socket>(mSink, std::move(socket));
      conn->asyncRead();
      conn->retrieveTraderId();
      spdlog::debug("{} connected", conn->getTraderId());
      mConnections.emplace(conn->getTraderId(), std::move(conn));
      acceptConnection();
    });
  }

private:
  ServerSink &mSink;
  TcpAcceptor mAcceptor;
  Port mPort;

  std::unordered_map<TraderId, Socket::UPtr> mConnections;
};
} // namespace hft::server::network

#endif // HFT_SERVER_NETWORK_INGRESSSERVER_HPP