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

template <typename BusT, typename... MessageTs>
struct BusLimiter;

template <typename BusT, typename... MessageTs>
struct BusRestrictor;

template <typename Parser>
class ConsoleReader;

template <typename Serializer>
class DummyFramer;

class ShmTransport;
class BoostTcpTransport;
class BoostUdpTransport;

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

#ifdef SERIALIZATION_SBE
using DomainSerializer = serialization::sbe::SbeDomainSerializer;
using Framer = DummyFramer<DomainSerializer>;
#else
using DomainSerializer = serialization::fbs::FbsDomainSerializer;
using Framer = FixedSizeFramer<DomainSerializer>;
#endif

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

struct ChannelStatusEvent;
struct ConnectionStatusEvent;
} // namespace hft

namespace hft::client {
class CommandParser;
class ShmClient;
class BoostNetworkClient;

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

#ifdef COMM_SHM
using StreamTransport = ShmTransport;
using DatagramTransport = ShmTransport;
using NetworkClient = ShmClient;
#else
using StreamTransport = BoostTcpTransport;
using DatagramTransport = BoostUdpTransport;
using NetworkClient = BoostNetworkClient;
#endif

using ClientMessageBus = MessageBus<Order, OrderStatus, TickerPrice>;
using ClientBus = BusHub<ClientMessageBus, ClientStreamBus>;
using UpstreamBus =
    BusRestrictor<ClientBus, LoginResponse, ChannelStatusEvent, ConnectionStatusEvent>;
using DownstreamBus =
    BusRestrictor<ClientBus, LoginResponse, OrderStatus, ChannelStatusEvent, ConnectionStatusEvent>;
using DatagramBus =
    BusRestrictor<ClientBus, TickerPrice, ChannelStatusEvent, ConnectionStatusEvent>;

using ClientConsoleReader = ConsoleReader<CommandParser>;

using StreamAdapter = MessageQueueAdapter<ClientBus, CommandParser>;
using DbAdapter = adapters::PostgresAdapter;

} // namespace hft::client

#endif // HFT_CLIENT_TRAITS_HPP
