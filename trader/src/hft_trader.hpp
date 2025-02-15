/**
 * @file
 * @brief
 *
 * @author Vladimir Pavliv
 * @date 2025-02-13
 */

#ifndef HFT_TRADER_HPP
#define HFT_TRADER_HPP

#include <chrono>

#include "control_center/control_center.hpp"
#include "network/network_server.hpp"
#include "trader_types.hpp"
#include "utils/string_utils.hpp"

namespace hft::trader {

class HftTrader {
public:
  HftTrader() : mCc{mSink}, mServer{mSink} {}

  void start() {
    spdlog::debug("Starting server");
    mSink.dataSink.setHandler<PriceUpdate>([this](const PriceUpdate &price) {
      spdlog::debug("PriceUpdate: {}", utils::toString(price));
    });
    mSink.dataSink.setHandler<OrderStatus>([this](const OrderStatus &status) {
      spdlog::debug("OrderStatus: {}", utils::toString(status));
    });
    mSink.controlSink.setHandler(TraderCommand::Shutdown, [this](TraderCommand cmd) {
      if (cmd == TraderCommand::Shutdown) {
        stop();
      }
    });
    mSink.networkSink.start();
    mSink.dataSink.start();
    mSink.controlSink.start();
    mServer.start();
    mCc.start();
  }

  void stop() {
    spdlog::debug("Stoping server");
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
};

} // namespace hft::trader

#endif // HFT_SERVER_HPP