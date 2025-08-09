/**
 * @author Vladimir Pavliv
 * @date 2025-04-18
 */

#ifndef HFT_CLIENT_CLIENTTYPES_HPP
#define HFT_CLIENT_CLIENTTYPES_HPP

#include "bus/bus_holder.hpp"
#include "domain_types.hpp"
#include "metadata_types.hpp"

namespace hft::client {

using ClientBus = BusHolder<MessageBus<Order, OrderStatus, TickerPrice>,
                            StreamBus<OrderTimestamp, RuntimeMetrics>>;
}

#endif // HFT_CLIENT_CLIENTTYPES_HPP