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

template <typename BusT, typename... MessageTs>
struct BusLimiter;

template <typename BusT, typename... MessageTs>
struct BusRestrictor;

template <typename MarketBusT = MessageBus<>>
struct BusHub;

template <typename Parser>
class ConsoleReader;

class ShmTransport;
class BoostTcpTransport;
class BoostUdpTransport;

template <typename Serializer>
class DummyFramer;

template <typename SerializerType>
class FixedSizeFramer;

template <typename BusT>
class TelemetryAdapter;

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

namespace hft::server {
class ShmServer;
class CommandParser;
class BoostIpcServer;
class NetworkSessionManager;
class TrustedSessionManager;

template <typename BusT>
using MessageQueueAdapter = adapters::DummyKafkaAdapter<BusT>;
using ClientStreamBus = StreamBus<LFQ_CAPACITY>;

#ifdef COMM_SHM
using StreamTransport = ShmTransport;
using DatagramTransport = ShmTransport;
using IpcServer = ShmServer;
using SessionManager = TrustedSessionManager;
#else
using StreamTransport = BoostTcpTransport;
using DatagramTransport = BoostUdpTransport;
using IpcServer = BoostIpcServer;
using SessionManager = NetworkSessionManager;
#endif

using ServerMessageBus = MessageBus<ServerOrder, ServerOrderStatus, TickerPrice>;
using ServerBus = BusHub<ServerMessageBus>;
using UpstreamBus = BusRestrictor<ServerBus, ServerOrder, ServerLoginRequest, ChannelStatusEvent,
                                  ConnectionStatusEvent>;
using DownstreamBus = BusRestrictor<ServerBus, ServerOrderStatus, ServerTokenBindRequest,
                                    ChannelStatusEvent, ConnectionStatusEvent>;
using DatagramBus =
    BusRestrictor<ServerBus, TickerPrice, ChannelStatusEvent, ConnectionStatusEvent>;

using ServerConsoleReader = ConsoleReader<CommandParser>;

using DbAdapter = adapters::PostgresAdapter;
using ServerTelemetryAdapter = TelemetryAdapter<ServerBus>;

} // namespace hft::server

#endif // HFT_SERVER_TRAITS_HPP
