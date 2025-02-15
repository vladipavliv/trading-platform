/**
 * @file
 * @brief
 *
 * @author Vladimir Pavliv
 * @date 2025-02-10
 */

#ifndef HFT_SERVER_NETWORK_EGRESSSERVER_HPP
#define HFT_SERVER_NETWORK_EGRESSSERVER_HPP

#include <unordered_map>

#include "config/config.hpp"
#include "server_types.hpp"
#include "types/market_types.hpp"
#include "types/network_types.hpp"
#include "types/types.hpp"
#include "utils/utils.hpp"

namespace hft::server::network {

class EgressServer {
public:
  using Socket = ServerSocket<TcpSocket, Order>;

  EgressServer(ServerSink &sink)
      : mSink{sink}, mAcceptor{mSink.ctx()}, mPort{Config::cfg.portTcpOut} {}

  void start() {
    spdlog::info("Start accepting egress connections on the port: {}", mPort);
    mSink.networkSink.setHandler<OrderStatus>([this](const OrderStatus &status) { send(status); });

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

  template <typename MessageType>
  void send(const MessageType &message) {
    auto conn = mConnections.find(message.traderId);
    if (conn == mConnections.end()) {
      spdlog::error("Failed to notify trader {}: not connected", message.traderId);
      return;
    }
    conn->second->asyncWrite(message);
  }

private:
  void acceptConnection() {
    mAcceptor.async_accept([this](BoostErrorRef ec, TcpSocket socket) {
      if (!ec) {
        auto traderId = utils::getTraderId(socket);
        if (mConnections.find(traderId) != mConnections.end()) {
          spdlog::error("Trader {} is already connected", traderId);
        } else {
          spdlog::debug("Accepted new egress connection from {}", traderId);
          auto conn = std::make_unique<Socket>(mSink, std::move(socket));
          mConnections.emplace(traderId, std::move(conn));
        }
      } else {
        spdlog::error("Failed to accept connection, error: {}", ec.message());
      }
      acceptConnection();
    });
  }

private:
  ServerSink &mSink;
  TcpAcceptor mAcceptor;
  Port mPort;

  // TODO(self) Investigate for cache locality.
  std::unordered_map<TraderId, Socket::UPtr> mConnections;
};
} // namespace hft::server::network

#endif // HFT_SERVER_NETWORK_EGRESSSERVER_HPP