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

#ifdef TELEMETRY_ENABLED
using StreamBusType = StreamBus<StreamBus<>::Capacity, OrderTimestamp, RuntimeMetrics>;
#else
using StreamBusType = StreamBus<>;
#endif

using ClientBus = BusHolder<MessageBus<Order, OrderStatus, TickerPrice>, StreamBusType>;
} // namespace hft::client

#endif // HFT_CLIENT_CLIENTTYPES_HPP
