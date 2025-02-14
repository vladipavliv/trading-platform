/**
 * @file
 * @brief
 *
 * @author Vladimir Pavliv
 * @date 2025-02-13
 */

#ifndef HFT_SERVER_MARKET_HPP
#define HFT_SERVER_MARKET_HPP

#include "server_types.hpp"

namespace hft::server::market {

class Market {
public:
  Market(ServerSink &sink) : mSink{sink} {}

  void start() {
    mSink.dataSink.setHandler<Order>([this](const Order &order) { processOrder(order); });
  }
  void stop() {}

private:
  void processOrder(const Order &order) {
    OrderStatus status{order.traderId, order.id, FulfillmentState::Full, order.quantity,
                       order.price};
    mSink.networkSink.post(status);
  }

private:
  ServerSink &mSink;
};

} // namespace hft::server::market

#endif // HFT_SERVER_MARKET_HPP