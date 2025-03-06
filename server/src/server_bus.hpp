/**
 * @author Vladimir Pavliv
 * @date 2025-03-06
 */

#ifndef HFT_SERVER_SERVERBUS_HPP
#define HFT_SERVER_SERVERBUS_HPP

#include "bus/command_bus.hpp"
#include "bus/event_bus.hpp"
#include "network/socket_status.hpp"
#include "server_command.hpp"

namespace hft::server {
struct ServerBus {
  using ServerCommandBus = CommandBus<ServerCommand>;
  using ServerEventBus = EventBus<Order, OrderStatus, TickerPrice, SocketStatusEvent>;

  static inline ServerCommandBus &commandBus() {
    static ServerCommandBus commandBus;
    return commandBus;
  }

  static inline ServerEventBus &eventBus() {
    static ServerEventBus eventBus;
    return eventBus;
  }
};
} // namespace hft::server

#endif // HFT_SERVER_SERVERBUS_HPP