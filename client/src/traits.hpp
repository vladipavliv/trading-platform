/**
 * @author Vladimir Pavliv
 * @date 2025-04-18
 */

#ifndef HFT_CLIENT_TRAITS_HPP
#define HFT_CLIENT_TRAITS_HPP

#include "constants.hpp"
#include "domain_types.hpp"

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

class ShmTransport;

template <typename SerializerType>
class FixedSizeFramer;

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
} // namespace hft

namespace hft::client {
class ShmClient;
class CommandParser;

using MetadataSerializer = serialization::proto::ProtoMetadataSerializer;

#ifdef TELEMETRY_ENABLED
template <typename BusT, typename ConsumeSerializerT = MetadataSerializer,
          typename ProduceSerializerT = MetadataSerializer>
using MessageQueueAdapter = adapters::KafkaAdapter<BusT, ConsumeSerializerT, ProduceSerializerT>;
using ClientStreamBus = StreamBus<LFQ_CAPACITY, OrderTimestamp, RuntimeMetrics>;
#else
template <typename BusT, typename ConsumeSerializerT = void, typename ProduceSerializerT = void>
using MessageQueueAdapter = adapters::DummyKafkaAdapter<BusT>;
using ClientStreamBus = StreamBus<LFQ_CAPACITY>;
#endif

#ifdef SERIALIZATION_SBE
using DomainSerializer = serialization::sbe::SbeDomainSerializer;
using Framer = DummyFramer<DomainSerializer>;
#else
using DomainSerializer = serialization::fbs::FbsDomainSerializer;
using Framer = FixedSizeFramer<DomainSerializer>;
#endif

using StreamTransport = ShmTransport;
using DatagramTransport = ShmTransport;
using NetworkClient = ShmClient;

using ClientMessageBus = MessageBus<Order, OrderStatus, TickerPrice>;
using ClientBus = BusHub<ClientMessageBus, ClientStreamBus>;

using ClientConsoleReader = ConsoleReader<CommandParser>;

using StreamAdapter = MessageQueueAdapter<ClientBus, CommandParser>;
using DbAdapter = adapters::PostgresAdapter;

} // namespace hft::client

#endif // HFT_CLIENT_TRAITS_HPP
