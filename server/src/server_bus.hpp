/**
 * @author Vladimir Pavliv
 * @date 2025-03-06
 */

#ifndef HFT_SERVER_SERVERBUS_HPP
#define HFT_SERVER_SERVERBUS_HPP

#include "boost_types.hpp"
#include "bus/message_bus.hpp"
#include "bus/system_bus.hpp"
#include "market_types.hpp"

namespace hft::server {

using MarketBus = MessageBus<Order, OrderStatus, TickerPrice>;

struct ServerBus {
  ServerBus(IoContext &systemIoCtx) : systemBus{systemIoCtx} {}

  SystemBus systemBus;
  MarketBus marketBus;
};

} // namespace hft::server

#endif // HFT_SERVER_SERVERBUS_HPP