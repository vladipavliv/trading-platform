/**
 * @file
 * @brief
 *
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

  ~IngressServer() { stop(); }

  void start() {
    spdlog::info("Start accepting ingress connections on the port: {}", mPort);

    TcpEndpoint endpoint(Tcp::v4(), mPort);

    mAcceptor.open(endpoint.protocol());
    mAcceptor.bind(endpoint);
    mAcceptor.listen();

    acceptConnection();
  }

  void stop() {
    mAcceptor.close();
    for (auto &conn : mConnections) {
      conn.second->close();
    }
  }

private:
  void acceptConnection() {
    mAcceptor.async_accept([this](BoostErrorRef ec, TcpSocket socket) {
      TraderId traderId = utils::getTraderId(socket);
      if (!ec) {
        if (mConnections.find(traderId) != mConnections.end()) {
          spdlog::error("Trader {} connected already", traderId);
        } else {
          spdlog::debug("Accepted new ingress connection from id: {}", traderId);

          auto conn = std::make_unique<Socket>(mSink, std::move(socket));
          conn->asyncRead();
          mConnections.emplace(traderId, std::move(conn));
        }
      } else {
        spdlog::error("Failed to accept connection {}", traderId);
      }
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