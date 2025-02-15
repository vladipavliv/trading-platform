/**
 * @file
 * @brief
 *
 * @author Vladimir Pavliv
 * @date 2025-02-13
 */

#ifndef HFT_SERVER_NETWORK_SERVER_HPP
#define HFT_SERVER_NETWORK_SERVER_HPP

#include "broadcast_server.hpp"
#include "egress_server.hpp"
#include "ingress_server.hpp"
#include "network_types.hpp"
#include "server_types.hpp"
#include "types.hpp"

namespace hft::server::network {

class NetworkServer {
public:
  NetworkServer(ServerSink &sink)
      : mSink{sink}, mIngressServer{mSink}, mEgressServer{mSink}, mMarketDataServer{mSink} {}

  void start() {
    mIngressServer.start();
    mEgressServer.start();
    mMarketDataServer.start();
  }

  void stop() {
    mIngressServer.stop();
    mEgressServer.stop();
    mMarketDataServer.stop();
  }

private:
  ServerSink &mSink;

  IngressServer mIngressServer;
  EgressServer mEgressServer;
  BroadcastServer mMarketDataServer;
};

} // namespace hft::server::network

#endif // HFT_SERVER_NETWORK_SERVER_HPP