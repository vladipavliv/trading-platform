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
      : mSink{sink}, mSocket{mSink, UdpEndpoint{Ip::address_v4::broadcast(), Config::cfg.portUdp}} {
  }
  ~BroadcastServer() { stop(); }

  void start() {
    mSocket.asyncConnect();
    mSink.networkSink.setHandler<PriceUpdate>(
        [this](const PriceUpdate &status) { mSocket.asyncWrite(status); });
  }

  void stop() { mSocket.close(); }

private:
  ServerSink &mSink;
  Socket mSocket;
};
} // namespace hft::server::network

#endif // HFT_SERVER_NETWORK_EGRESSSERVER_HPP