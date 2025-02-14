/**
 * @file
 * @brief
 *
 * @author Vladimir Pavliv
 * @date 2025-02-10
 */

#ifndef HFT_SERVER_NETWORK_BROADCASTSERVER_HPP
#define HFT_SERVER_NETWORK_BROADCASTSERVER_HPP

#include <spdlog/spdlog.h>
#include <unordered_map>

#include "boost_types.hpp"
#include "config/config.hpp"
#include "server_types.hpp"
#include "types/market_types.hpp"
#include "types/network_types.hpp"
#include "types/types.hpp"
#include "utils/utils.hpp"

namespace hft::server::network {

class BroadcastServer {
public:
  using Socket = ServerSocket<UdpSocket, PriceUpdate>;

  BroadcastServer(ServerSink &sink)
      : mSink{sink}, mEndpoint{Ip::address::from_string(Config::config().server.url),
                               Config::config().server.portUdp},
        mSocket{mSink.dataSink, mEndpoint} {}

  void start() {
    mSink.networkSink.setHandler<PriceUpdate>([this](const PriceUpdate &status) { /*send*/ });
    // mSocket.set_option(boost::asio::socket_base::broadcast(true));
  }

  void stop() {}

private:
  void sendHandler(BoostErrorRef ec, std::size_t size) {
    if (ec) {
      spdlog::error("Failed to send broadcast message: {}", ec.message());
    }
  }

private:
  ServerSink &mSink;
  UdpEndpoint mEndpoint;
  Socket mSocket;
};
} // namespace hft::server::network

#endif // HFT_SERVER_NETWORK_EGRESSSERVER_HPP