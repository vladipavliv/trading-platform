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
#include "sink/io_sink.hpp"
#include "sink/pool_event_sink.hpp"

namespace hft::trader {

using EventSink = PoolEventSink<OrderStatus, PriceUpdate>;
using NetworkSink = IoSink<Order>;
using ControlSink = CommandSink<TraderCommand>;

using Serializer = hft::serialization::FlatBuffersSerializer;

struct TraderSink { // SinkSink
  EventSink dataSink;
  NetworkSink networkSink;
  ControlSink controlSink;
  inline IoContext &ctx() { return networkSink.ctx(); }
};

template <typename SocketType, typename MessageType>
using TraderSocket = BufferedSocket<TraderSink, Serializer, SocketType, MessageType>;

} // namespace hft::trader

#endif // HFT_TRADER_SERVERTYPES_HPP