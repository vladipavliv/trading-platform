/**
 * @author Vladimir Pavliv
 * @date 2025-03-06
 */

#ifndef HFT_SERVER_TRADERBUS_HPP
#define HFT_SERVER_TRADERBUS_HPP

#include "bus/command_bus.hpp"
#include "bus/event_bus.hpp"
#include "network/socket_status.hpp"
#include "trader_command.hpp"

namespace hft::trader {
struct TraderBus {
  using TraderCommandBus = CommandBus<TraderCommand>;
  using TraderEventBus = EventBus<Order, OrderStatus, TickerPrice, SocketStatusEvent>;

  static inline TraderCommandBus &commandBus() {
    static TraderCommandBus commandBus;
    return commandBus;
  }

  static inline TraderEventBus &eventBus() {
    static TraderEventBus eventBus;
    return eventBus;
  }
};
} // namespace hft::trader

#endif // HFT_SERVER_TRADERBUS_HPP