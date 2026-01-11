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

template <typename MarketBusT>
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
class BoostIpcClient;
class NetworkConnectionManager;
class TrustedConnectionManager;

template <typename BusT>
using MessageQueueAdapter = adapters::DummyKafkaAdapter<BusT>;
using ClientStreamBus = StreamBus<LFQ_CAPACITY>;

#ifdef COMM_SHM
using StreamTransport = ShmTransport;
using DatagramTransport = ShmTransport;
using IpcClient = ShmClient;
using ConnectionManager = TrustedConnectionManager;
#else
using StreamTransport = BoostTcpTransport;
using DatagramTransport = BoostUdpTransport;
using IpcClient = BoostIpcClient;
using ConnectionManager = NetworkConnectionManager;
#endif

using ClientMessageBus = MessageBus<Order, OrderStatus, TickerPrice>;
using ClientBus = BusHub<ClientMessageBus>;
using UpstreamBus =
    BusRestrictor<ClientBus, LoginResponse, ChannelStatusEvent, ConnectionStatusEvent>;
using DownstreamBus =
    BusRestrictor<ClientBus, LoginResponse, OrderStatus, ChannelStatusEvent, ConnectionStatusEvent>;
using DatagramBus =
    BusRestrictor<ClientBus, TickerPrice, ChannelStatusEvent, ConnectionStatusEvent>;

using ClientConsoleReader = ConsoleReader<CommandParser>;
using DbAdapter = adapters::PostgresAdapter;

} // namespace hft::client

#endif // HFT_CLIENT_TRAITS_HPP
