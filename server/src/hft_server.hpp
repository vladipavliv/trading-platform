/**
 * @author Vladimir Pavliv
 * @date 2025-02-13
 */

#ifndef HFT_SERVER_HPP
#define HFT_SERVER_HPP

#include "aggregator/aggregator.hpp"
#include "control_center/control_center.hpp"
#include "network/network_server.hpp"
#include "server_types.hpp"

namespace hft::server {

class HftServer {
public:
  HftServer() : mNetwork{mSink}, mCc{mSink}, mAggregator{mSink} {
    mSink.controlSink.addCommandHandler({ServerCommand::Shutdown}, [this](ServerCommand cmd) {
      if (cmd == ServerCommand::Shutdown) {
        stop();
      }
    });
  }

  void start() {
    mSink.ioSink.start();
    mSink.dataSink.start();
    mSink.controlSink.start();
    mNetwork.start();
    mAggregator.start();
    // Run io context along with other threads
    mSink.ctx().run();
  }

  void stop() {
    mSink.ctx().stop();
    mSink.dataSink.stop();
  }

private:
  ServerSink mSink;
  network::NetworkServer mNetwork;
  Aggregator mAggregator;
  ControlCenter mCc;
};

} // namespace hft::server

#endif // HFT_SERVER_HPP