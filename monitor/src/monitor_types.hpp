/**
 * @author Vladimir Pavliv
 * @date 2025-08-12
 */

#ifndef HFT_MONITOR_MONITORTYPES_HPP
#define HFT_MONITOR_MONITORTYPES_HPP

#include "bus/bus_holder.hpp"
#include "domain_types.hpp"
#include "metadata_types.hpp"

namespace hft::monitor {

using MonitorBus = BusHolder<MessageBus<>, StreamBus<OrderTimestamp, RuntimeMetrics>>;
}

#endif // HFT_MONITOR_MONITORTYPES_HPP