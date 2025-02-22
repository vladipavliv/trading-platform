/**
 * @author Vladimir Pavliv
 * @date 2025-02-13
 */

#ifndef HFT_SERVER_SERVERTYPES_HPP
#define HFT_SERVER_SERVERTYPES_HPP

#include "control_center/server_commands.hpp"
#include "market_types.hpp"
#include "network/async_socket.hpp"
#include "serialization/flat_buffers/fb_serializer.hpp"
#include "sink/batch_io_sink.hpp"
#include "sink/command_sink.hpp"
#include "sink/io_sink.hpp"
#include "sink/partition_event_sink.hpp"
#include "sink/pool_event_sink.hpp"

namespace hft::server {

class Aggregator;
class OrderTrafficStats;
class MapOrderBook;
class FlatOrderBook;

using EventSink = PartitionEventSink<Aggregator, Order>;
using ServerIoSink = IoSink<OrderStatus, TickerPrice>;
using ServerControlSink = ControlSink<ServerCommand, OrderTrafficStats>;
using Serializer = hft::serialization::FlatBuffersSerializer;
using OrderBook = MapOrderBook;

struct ServerSink {
  EventSink dataSink;
  ServerIoSink ioSink;
  ServerControlSink controlSink;
  inline IoContext &ctx() { return ioSink.ctx(); }
};

template <typename SocketType, typename MessageType>
using ServerSocket = AsyncSocket<ServerSink, Serializer, SocketType, MessageType>;

} // namespace hft::server

#endif // HFT_SERVER_SERVERTYPES_HPP