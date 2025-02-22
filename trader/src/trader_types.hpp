/**
 * @author Vladimir Pavliv
 * @date 2025-02-13
 */

#ifndef HFT_TRADER_SERVERTYPES_HPP
#define HFT_TRADER_SERVERTYPES_HPP

#include "control_center/trader_commands.hpp"
#include "market_types.hpp"
#include "network/async_socket.hpp"
#include "serialization/flat_buffers/fb_serializer.hpp"
#include "sink/buffer_io_sink.hpp"
#include "sink/command_sink.hpp"
#include "sink/io_sink.hpp"
#include "sink/partition_event_sink.hpp"
#include "sink/pool_event_sink.hpp"

namespace hft::trader {

struct TradingStats;

using EventSink = PoolEventSink<OrderStatus, TickerPrice>;
using TraderIoSink = BufferIoSink<Order>;
using TraderControlSink = ControlSink<TraderCommand, TradingStats>;

using Serializer = hft::serialization::FlatBuffersSerializer;

struct TraderSink {
  EventSink dataSink;
  TraderIoSink ioSink;
  TraderControlSink controlSink;
  inline IoContext &ctx() { return ioSink.ctx(); }
};

template <typename SocketType, typename MessageType>
using TraderSocket = AsyncSocket<TraderSink, Serializer, SocketType, MessageType>;

} // namespace hft::trader

#endif // HFT_TRADER_SERVERTYPES_HPP