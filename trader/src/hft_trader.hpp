/**
 * @file
 * @brief
 *
 * @author Vladimir Pavliv
 * @date 2025-02-13
 */

#ifndef HFT_TRADER_HPP
#define HFT_TRADER_HPP

#include "control_center/control_center.hpp"
#include "network/network_server.hpp"
#include "trader_types.hpp"

namespace hft::trader {

class HftTrader {
public:
  HftTrader() : mCc{mSink.controlSink} {}

  void start() {
    mSink.controlSink.setHandler(TraderCommand::Shutdown, [this](TraderCommand cmd) {
      if (cmd == TraderCommand::Shutdown) {
        stop();
      }
    });
    mSink.networkSink.start();
    mSink.dataSink.start();
    mSink.controlSink.start();
    mCc.start();
  }

  void stop() {
    mSink.networkSink.stop();
    mSink.dataSink.stop();
    mSink.controlSink.stop();
    mCc.stop();
  }

private:
  TraderSink mSink;
  ControlCenter mCc;
};

} // namespace hft::trader

#endif // HFT_SERVER_HPP