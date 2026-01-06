/**
 * @author Vladimir Pavliv
 * @date 2025-08-12
 */

#ifndef HFT_MONITOR_TRAITS_HPP
#define HFT_MONITOR_TRAITS_HPP

#include "bus/bus_hub.hpp"
#include "constants.hpp"
#include "domain_types.hpp"
#include "metadata_types.hpp"

namespace hft {
template <typename... Events>
class MessageBus;

template <size_t Capacity, typename... Events>
class StreamBus;

template <typename MarketBusT = MessageBus<>, typename StreamBusT = StreamBus<LFQ_CAPACITY>>
struct BusHub;

template <typename Parser>
class ConsoleReader;

template <typename Serializer>
class DummyFramer;

namespace serialization {
namespace proto {
class ProtoMetadataSerializer;
}
namespace fbs {
class FbsDomainSerializer;
}
namespace sbe {
class SbeDomainSerializer;
}
} // namespace serialization

namespace adapters {
template <typename BusT,
          typename ConsumeSerializerT = serialization::proto::ProtoMetadataSerializer,
          typename ProduceSerializerT = serialization::proto::ProtoMetadataSerializer>
class KafkaAdapter;
template <typename BusType>
class DummyKafkaAdapter;
class PostgresAdapter;
} // namespace adapters

namespace serialization {
namespace proto {
class ProtoMetadataSerializer;
}
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

using MetadataSerializer = serialization::proto::ProtoMetadataSerializer;

using MonitorBus = BusHub<MessageBus<>, StreamBus<LFQ_CAPACITY, OrderTimestamp, RuntimeMetrics>>;

template <typename BusT, typename ConsumeSerializerT, typename ProduceSerializerT>
using MessageQueueAdapter = adapters::KafkaAdapter<BusT, ConsumeSerializerT, ProduceSerializerT>;

using StreamAdapter = MessageQueueAdapter<MonitorBus, MetadataSerializer, CommandParser>;
using MonitorConsoleReader = ConsoleReader<CommandParser>;

} // namespace hft::monitor

#endif // HFT_MONITOR_TRAITS_HPP
