/**
 * @author Vladimir Pavliv
 * @date 2025-03-21
 */

#ifndef HFT_COMMON_BUS_HPP
#define HFT_COMMON_BUS_HPP

#include "market_types.hpp"
#include "message_bus.hpp"
#include "system_bus.hpp"

namespace hft {
using MarketBus = MessageBus<Order, OrderStatus, TickerPrice>;

struct Bus {
  SystemBus systemBus;
  MarketBus marketBus;

  inline IoContext &ioCtx() { return systemBus.systemIoCtx; }
  void run() { systemBus.systemIoCtx.run(); }
  void stop() { systemBus.systemIoCtx.stop(); }
};
} // namespace hft

#endif // HFT_COMMON_BUS_HPP