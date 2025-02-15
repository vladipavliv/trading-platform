/**
 * @file
 * @brief
 *
 * @author Vladimir Pavliv
 * @date 2025-02-13
 */

#ifndef HFT_SERVER_SERVERTYPES_HPP
#define HFT_SERVER_SERVERTYPES_HPP

#include "control_center/server_commands.hpp"
#include "market_types.hpp"
#include "network/buffered_socket.hpp"
#include "serialization/flat_buffers/fb_serializer.hpp"
#include "sink/command_sink.hpp"
#include "sink/event_sink.hpp"
#include "sink/io_sink.hpp"

namespace hft::server {

constexpr uint8_t EVENT_THREAD_COUNT = 1;
constexpr uint8_t IO_THREAD_COUNT = 1;
constexpr uint16_t EVENT_QUEUE_SIZE = 1024;

using DataSink = EventSink<EVENT_THREAD_COUNT, EVENT_QUEUE_SIZE, Order>;
using NetworkSink = IoSink<IO_THREAD_COUNT, OrderStatus, PriceUpdate>;
using ControlSink = CommandSink<ServerCommand>;

using Serializer = hft::serialization::FlatBuffersSerializer;

struct ServerSink { // SinkSink
  DataSink dataSink;
  NetworkSink networkSink;
  ControlSink controlSink;
  inline IoContext &ctx() { return networkSink.ctx(); }
};

template <typename SocketType, typename MessageType>
using ServerSocket = BufferedSocket<ServerSink, Serializer, SocketType, MessageType>;

} // namespace hft::server

#endif // HFT_SERVER_SERVERTYPES_HPP