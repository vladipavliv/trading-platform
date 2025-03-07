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
struct TraderBus {
  using MarketBus = MessageBus<Order, OrderStatus, TickerPrice>;

  static SystemBus systemBus;
  static MarketBus marketBus;
};
SystemBus TraderBus::systemBus;
TraderBus::MarketBus TraderBus::marketBus;
} // namespace hft::trader

#endif // HFT_SERVER_TRADERBUS_HPP