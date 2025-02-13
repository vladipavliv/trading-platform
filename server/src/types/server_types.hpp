/**
 * @file
 * @brief
 *
 * @author Vladimir Pavliv
 * @date 2025-02-13
 */

#ifndef HFT_SERVER_SERVERTYPES_HPP
#define HFT_SERVER_SERVERTYPES_HPP

#include "market_types.hpp"
#include "network/buffered_socket.hpp"
#include "serialization/flat_buffers/fb_serializer.hpp"
#include "server_commands.hpp"
#include "sink/command_sink.hpp"
#include "sink/event_sink.hpp"
#include "sink/io_sink.hpp"

namespace hft::server {

constexpr uint8_t EVENT_THREAD_COUNT = 4;
constexpr uint8_t IO_THREAD_COUNT = 4;
constexpr uint16_t EVENT_QUEUE_SIZE = 1024;

using DataSink = EventSink<EVENT_THREAD_COUNT, EVENT_QUEUE_SIZE, Order>;
using Serializer = hft::serialization::FlatBuffersSerializer;
using CmdSink = CommandSink<ServerCommand>;

using RingSocket = BufferedSocket<DataSink, Serializer, Order, OrderStatus, PriceUpdate>;

struct ServerSink { // SinkSink
  DataSink dataSink;
  IoSink ioSink;
  CmdSink cmdSink;
};

} // namespace hft::server

#endif // HFT_SERVER_SERVERTYPES_HPP