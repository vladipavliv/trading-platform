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
  HftServer() : mNetwork{mSink}, mCc{mSink.controlSink}, mMarket{mSink} {}

  void start() {
    mSink.controlSink.setHandler(ServerCommand::Shutdown, [this](ServerCommand cmd) {
      if (cmd == ServerCommand::Shutdown) {
        stop();
      }
    });
    mSink.networkSink.start();
    mSink.dataSink.start();
    mSink.controlSink.start();
    mNetwork.start();
    mMarket.start();
    mCc.start();
  }

  void stop() {
    mSink.networkSink.stop();
    mSink.dataSink.stop();
    mSink.controlSink.stop();
    mNetwork.stop();
    mMarket.stop();
    mCc.stop();
  }

private:
  ServerSink mSink;
  network::NetworkServer mNetwork;
  market::Market mMarket;
  ControlCenter mCc;
};

} // namespace hft::server

#endif // HFT_SERVER_HPP