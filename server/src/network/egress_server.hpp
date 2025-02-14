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
  EgressServer(ServerSink &sink)
      : mSink{sink}, mAcceptor{mSink.networkSink.ctx()}, mPort{Config::config().server.portTcpOut} {
  }

  void run() {
    TcpEndpoint endpoint(Tcp::v4(), mPort);

    mAcceptor.open(endpoint.protocol());
    mAcceptor.bind(endpoint);
    mAcceptor.listen();

    acceptConnection();
  }

  template <typename MessageType>
  void send(MessageType &&message) {
    auto conn = mConnections.find(message.traderId);
    if (conn == mConnections.end()) {
      spdlog::error("Failed to notify trader {}: not connected", message.traderId);
      return;
    }
    conn->second->send(std::forward<MessageType>(message));
  }

  void stop() {
    mAcceptor.close();
    for (auto &conn : mConnections) {
      conn.second->close();
    }
  }

private:
  void acceptConnection() {
    mAcceptor.async_accept([this](const boost::system::error_code &ec, TcpSocket socket) {
      auto traderId = utils::getTraderId(socket);
      if (!ec) {
        if (mConnections.find(traderId) != mConnections.end()) {
          spdlog::error("Trader {} is already connected", traderId);
        } else {
          spdlog::debug("Accepted new egress connection from {}", traderId);
          auto conn = std::make_unique<RingSocket>(mSink.dataSink, std::move(socket));
          mConnections.emplace(traderId, std::move(conn));
        }
      } else {
        spdlog::error("Failed to accept connection {}, error: ", traderId, ec.message());
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

#endif // HFT_SERVER_NETWORK_EGRESSSERVER_HPP