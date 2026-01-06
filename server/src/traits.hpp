/**
 * @author Vladimir Pavliv
 * @date 2025-04-18
 */

#ifndef HFT_SERVER_TRAITS_HPP
#define HFT_SERVER_TRAITS_HPP

#include "constants.hpp"
#include "server_domain_types.hpp"

namespace hft {
template <typename... Events>
class MessageBus;

template <size_t Capacity, typename... Events>
class StreamBus;

template <typename MarketBusT = MessageBus<>, typename StreamBusT = StreamBus<LFQ_CAPACITY>>
struct BusHub;

template <typename Parser>
class ConsoleReader;

class ShmTransport;

template <typename Serializer>
class DummyFramer;

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

namespace hft::server {
class ShmServer;
class CommandParser;

using MetadataSerializer = serialization::proto::ProtoMetadataSerializer;

#ifdef TELEMETRY_ENABLED
template <typename BusT, typename ConsumeSerializerT = MetadataSerializer,
          typename ProduceSerializerT = MetadataSerializer>
using MessageQueueAdapter = adapters::KafkaAdapter<BusT, ConsumeSerializerT, ProduceSerializerT>;
using ServerStreamBus = StreamBus<LFQ_CAPACITY, OrderTimestamp, RuntimeMetrics>;
#else
template <typename BusT, typename ConsumeSerializerT = void, typename ProduceSerializerT = void>
using MessageQueueAdapter = adapters::DummyKafkaAdapter<BusT>;
using ServerStreamBus = StreamBus<LFQ_CAPACITY>;
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
using NetworkServer = ShmServer;

using ServerMessageBus = MessageBus<ServerOrder, ServerOrderStatus, TickerPrice>;
using ServerBus = BusHub<ServerMessageBus, ServerStreamBus>;

using ServerConsoleReader = ConsoleReader<CommandParser>;

using StreamAdapter = MessageQueueAdapter<ServerBus, CommandParser>;
using DbAdapter = adapters::PostgresAdapter;

} // namespace hft::server

#endif // HFT_SERVER_TRAITS_HPP
