/**
 * @author Vladimir Pavliv
 * @date 2025-03-06
 */

#ifndef HFT_SERVER_SERVERBUS_HPP
#define HFT_SERVER_SERVERBUS_HPP

#include "bus/message_bus.hpp"
#include "bus/system_bus.hpp"
#include "market_types.hpp"

namespace hft::server {
struct ServerBus {
  using MarketBus = MessageBus<Order, OrderStatus, TickerPrice>;

  static SystemBus systemBus;
  static MarketBus marketBus;
};

SystemBus ServerBus::systemBus;
ServerBus::MarketBus ServerBus::marketBus;
} // namespace hft::server

#endif // HFT_SERVER_SERVERBUS_HPP