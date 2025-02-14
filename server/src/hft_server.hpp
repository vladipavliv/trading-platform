/**
 * @file
 * @brief
 *
 * @author Vladimir Pavliv
 * @date 2025-02-13
 */

#ifndef HFT_SERVER_HPP
#define HFT_SERVER_HPP

#include "control_center/control_center.hpp"
#include "market/market.hpp"
#include "network/network_server.hpp"
#include "server_types.hpp"

namespace hft::server {

class HftServer {
public:
  HftServer() : mNetwork{mSink}, mCc{mSink.controlSink}, mMarket{mSink.dataSink} {}

  void start() {
    // TODO(self): Register for commands in control sink
    mSink.networkSink.start();
    mSink.dataSink.start();
    mSink.controlSink.start();
    mNetwork.start();
    mCc.start();
  }

private:
  ServerSink mSink;
  network::NetworkServer mNetwork;
  market::Market mMarket;
  ControlCenter mCc;
};

} // namespace hft::server

#endif // HFT_SERVER_HPP