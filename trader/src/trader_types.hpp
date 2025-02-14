/**
 * @file
 * @brief
 *
 * @author Vladimir Pavliv
 * @date 2025-02-13
 */

#ifndef HFT_TRADER_SERVERTYPES_HPP
#define HFT_TRADER_SERVERTYPES_HPP

#include "control_center/trader_commands.hpp"
#include "market_types.hpp"
#include "network/buffered_socket.hpp"
#include "serialization/flat_buffers/fb_serializer.hpp"
#include "sink/command_sink.hpp"
#include "sink/event_sink.hpp"
#include "sink/io_sink.hpp"

namespace hft::trader {

constexpr uint8_t EVENT_THREAD_COUNT = 1;
constexpr uint8_t NETWORK_THREAD_COUNT = 1;
constexpr uint16_t EVENT_QUEUE_SIZE = 1024;

using DataSink = EventSink<EVENT_THREAD_COUNT, EVENT_QUEUE_SIZE, OrderStatus, PriceUpdate>;
using NetworkSink = IoSink<NETWORK_THREAD_COUNT, Order>;
using ControlSink = CommandSink<TraderCommand>;

using Serializer = hft::serialization::FlatBuffersSerializer;

struct TraderSink { // SinkSink
  DataSink dataSink;
  NetworkSink networkSink;
  ControlSink controlSink;
};

template <typename SocketType, typename MessageType>
using TraderSocket = BufferedSocket<TraderSink, Serializer, SocketType, MessageType>;

} // namespace hft::trader

#endif // HFT_TRADER_SERVERTYPES_HPP