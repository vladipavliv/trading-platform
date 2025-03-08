/**
 * @author Vladimir Pavliv
 * @date 2025-03-06
 */

#ifndef HFT_SERVER_TRADERBUS_HPP
#define HFT_SERVER_TRADERBUS_HPP

#include "bus/message_bus.hpp"
#include "bus/system_bus.hpp"
#include "market_types.hpp"

namespace hft::trader {
using MarketBus = MessageBus<Order, OrderStatus, TickerPrice>;

struct TraderBus {
  SystemBus systemBus;
  MarketBus marketBus;

  inline IoContext &ioCtx() { return systemBus.systemIoCtx; }
  void run() { systemBus.systemIoCtx.run(); }
  void stop() { systemBus.systemIoCtx.stop(); }
};
} // namespace hft::trader

#endif // HFT_SERVER_TRADERBUS_HPP