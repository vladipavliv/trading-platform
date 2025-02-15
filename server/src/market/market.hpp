/**
 * @file
 * @brief
 *
 * @author Vladimir Pavliv
 * @date 2025-02-13
 */

#ifndef HFT_SERVER_MARKET_HPP
#define HFT_SERVER_MARKET_HPP

#include <spdlog/spdlog.h>

#include "market_types.hpp"
#include "server_types.hpp"
#include "utils/utils.hpp"

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
    spdlog::debug("Order processed {}", utils::toString(order));
    OrderStatus status{order.traderId, order.id, FulfillmentState::Full, order.quantity,
                       order.price};
    mSink.networkSink.post(status);
  }

private:
  ServerSink &mSink;
};

} // namespace hft::server::market

#endif // HFT_SERVER_MARKET_HPP
