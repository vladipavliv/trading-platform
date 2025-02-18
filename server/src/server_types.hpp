/**
 * @author Vladimir Pavliv
 * @date 2025-02-13
 */

#ifndef HFT_SERVER_SERVERTYPES_HPP
#define HFT_SERVER_SERVERTYPES_HPP

#include "control_center/server_commands.hpp"
#include "market_types.hpp"
#include "network/buffered_socket.hpp"
#include "serialization/flat_buffers/fb_serializer.hpp"
#include "sink/balancing_event_sink.hpp"
#include "sink/command_sink.hpp"
#include "sink/io_sink.hpp"
#include "sink/pool_event_sink.hpp"

namespace hft::server {

class Market;

using EventSink = BalancingEventSink<Market, Order>;
// PoolEventSink<Order>;

using NetworkSink = IoSink<OrderStatus, TickerPrice>;
using ControlSink = CommandSink<ServerCommand>;

using Serializer = hft::serialization::FlatBuffersSerializer;

struct ServerSink {
  EventSink dataSink;
  NetworkSink networkSink;
  ControlSink controlSink;
  inline IoContext &ctx() { return networkSink.ctx(); }
};

template <typename SocketType, typename MessageType>
using ServerSocket = BufferedSocket<ServerSink, Serializer, SocketType, MessageType>;

} // namespace hft::server

#endif // HFT_SERVER_SERVERTYPES_HPP