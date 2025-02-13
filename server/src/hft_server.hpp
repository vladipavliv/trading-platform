/**
 * @file
 * @brief
 *
 * @author Vladimir Pavliv
 * @date 2025-02-13
 */

#ifndef HFT_SERVER_HPP
#define HFT_SERVER_HPP

#include "market/market.hpp"
#include "network/network_server.hpp"
#include "server_types.hpp"

namespace hft::server {

class HftServer {
public:
  HftServer() : mNetworkServer{mSink} {}

  void start() {
    mSink.ioSink.start();
    mSink.dataSink.start();
    mSink.cmdSink.start();
    mNetworkServer.start();
  }

private:
  ServerSink mSink;
  network::NetworkServer mNetworkServer;
  market::Market mMarket;
};

} // namespace hft::server

#endif // HFT_SERVER_HPP