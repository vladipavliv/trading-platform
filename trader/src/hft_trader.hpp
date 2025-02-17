/**
 * @author Vladimir Pavliv
 * @date 2025-02-13
 */

#ifndef HFT_TRADER_HPP
#define HFT_TRADER_HPP

#include <chrono>

#include "control_center/control_center.hpp"
#include "network/network_server.hpp"
#include "strategy/buy_some_sell_some_strategy.hpp"
#include "trader_types.hpp"
#include "utils/string_utils.hpp"

namespace hft::trader {

class HftTrader {
public:
  HftTrader() : mServer{mSink}, mCc{mSink}, mStrategy{mSink} {
    mSink.controlSink.setHandler(TraderCommand::Shutdown, [this](TraderCommand cmd) {
      if (cmd == TraderCommand::Shutdown) {
        stop();
      }
    });
  }

  void start() {
    mSink.networkSink.start();
    mSink.dataSink.start();
    mSink.controlSink.start();
    mServer.start();
    mCc.start();
  }

  void stop() {
    mSink.networkSink.stop();
    mSink.dataSink.stop();
    mSink.controlSink.stop();
    mServer.stop();
    mCc.stop();
  }

private:
  TraderSink mSink;
  NetworkServer mServer;
  ControlCenter mCc;
  BuySomeSellSomeStrategy mStrategy;
};

} // namespace hft::trader

#endif // HFT_SERVER_HPP