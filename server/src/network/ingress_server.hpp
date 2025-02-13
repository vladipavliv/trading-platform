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
  IngressServer(ServerSink &sink)
      : mSink{sink}, mAcceptor{mSink.ioSink.ctx()}, mPort{Config::config().server.portTcpIn} {}

  ~IngressServer() { stop(); }

  void run() {
    // TODO(do) Register for command sink for disconnect

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
          spdlog::debug("Accepted new ingress connection from {}", traderId);

          auto conn = std::make_unique<RingSocket>(mSink.dataSink, std::move(socket));
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

  std::unordered_map<TraderId, RingSocket::UPtr> mConnections;
};
} // namespace hft::server::network

#endif // HFT_SERVER_NETWORK_INGRESSSERVER_HPP