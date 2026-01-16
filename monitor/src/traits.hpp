/**
 * @author Vladimir Pavliv
 * @date 2025-08-12
 */

#ifndef HFT_MONITOR_TRAITS_HPP
#define HFT_MONITOR_TRAITS_HPP

#include "bus/bus_hub.hpp"
#include "constants.hpp"
#include "domain_types.hpp"
#include "types/telemetry_types.hpp"

namespace hft {
template <typename... Events>
class MessageBus;
template <size_t Capacity, typename... Events>
class StreamBus;
template <typename MarketBusT = MessageBus<>>
struct BusHub;

template <typename Parser>
class ConsoleReader;

template <typename Serializer>
class DummyFramer;

template <typename BusT>
class TelemetryAdapter;
template <typename BusT>
class DummyTelemetryAdapter;

namespace serialization {
namespace fbs {
class FbsDomainSerializer;
}
namespace sbe {
class SbeDomainSerializer;
}
} // namespace serialization

namespace adapters {
template <typename BusType>
class DummyKafkaAdapter;
class PostgresAdapter;
} // namespace adapters

namespace serialization {
namespace fbs {
class FbsDomainSerializer;
}
namespace sbe {
class SbeDomainSerializer;
}
} // namespace serialization
} // namespace hft

namespace hft::monitor {
class CommandParser;

using MonitorBus = BusHub<MessageBus<TelemetryMsg>>;

using MonitorConsoleReader = ConsoleReader<CommandParser>;

#ifndef CICD
using MonitorTelemetry = TelemetryAdapter<MonitorBus>;
#else
using MonitorTelemetry = DummyTelemetryAdapter<MonitorBus>;
#endif

} // namespace hft::monitor

#endif // HFT_MONITOR_TRAITS_HPP
