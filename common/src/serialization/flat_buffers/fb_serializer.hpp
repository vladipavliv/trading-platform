/**
 * @author Vladimir Pavliv
 * @date 2025-02-13
 */

#ifndef HFT_COMMON_SERIALIZATION_FBSERIALIZER_HPP
#define HFT_COMMON_SERIALIZATION_FBSERIALIZER_HPP

#include <spdlog/spdlog.h>

#include "converter.hpp"
#include "gen/marketdata_generated.h"
#include "types/market_types.hpp"
#include "types/result.hpp"
#include "types/types.hpp"

namespace hft::serialization {

/**
 * @brief Serializer via flat buffers
 * @todo Try SBE or Cap'N'Proto
 */
class FlatBuffersSerializer {
public:
  using DetachedBuffer = flatbuffers::DetachedBuffer;

  template <typename MessageType>
  static std::enable_if_t<std::is_same<MessageType, Order>::value, Result<Order>>
  deserialize(const uint8_t *buffer, size_t size) {
    flatbuffers::Verifier verifier(buffer, size);
    if (!verifier.VerifyBuffer<gen::fbs::Order>()) {
      spdlog::error("Order verification failed");
      return StatusCode::Error;
    }
    auto msg = flatbuffers::GetRoot<gen::fbs::Order>(buffer);
    return Order{0,
                 msg->id(),
                 fbStringToTicker(msg->ticker()),
                 msg->quantity(),
                 msg->price(),
                 convert(msg->action())};
  }

  template <typename MessageType>
  static std::enable_if_t<std::is_same<MessageType, OrderStatus>::value, Result<OrderStatus>>
  deserialize(const uint8_t *buffer, size_t size) {
    flatbuffers::Verifier verifier(buffer, size);
    if (!verifier.VerifyBuffer<gen::fbs::OrderStatus>()) {
      spdlog::error("OrderStatus verification failed");
      return StatusCode::Error;
    }
    auto orderMsg = flatbuffers::GetRoot<gen::fbs::OrderStatus>(buffer);
    return OrderStatus{0,
                       orderMsg->id(),
                       fbStringToTicker(orderMsg->ticker()),
                       orderMsg->quantity(),
                       orderMsg->fill_price(),
                       convert(orderMsg->state()),
                       convert(orderMsg->action())};
  }

  template <typename MessageType>
  static std::enable_if_t<std::is_same<MessageType, TickerPrice>::value, Result<TickerPrice>>
  deserialize(const uint8_t *buffer, size_t size) {
    flatbuffers::Verifier verifier(buffer, size);
    if (!verifier.VerifyBuffer<gen::fbs::TickerPrice>()) {
      spdlog::error("TickerPrice verification failed");
      return StatusCode::Error;
    }
    auto orderMsg = flatbuffers::GetRoot<gen::fbs::TickerPrice>(buffer);
    return TickerPrice{fbStringToTicker(orderMsg->ticker()), orderMsg->price()};
  }

  /**
   * @todo Could serialize the message directly into allocated for async send buffer
   */
  static DetachedBuffer serialize(const Order &order) {
    flatbuffers::FlatBufferBuilder builder;
    auto msg = gen::fbs::CreateOrder(builder, order.id,
                                     builder.CreateString(order.ticker.data(), TICKER_SIZE),
                                     order.quantity, order.price, convert(order.action));
    builder.Finish(msg);
    return builder.Release();
  }

  static DetachedBuffer serialize(const OrderStatus &order) {
    flatbuffers::FlatBufferBuilder builder;
    auto msg = gen::fbs::CreateOrderStatus(
        builder, order.id, builder.CreateString(order.ticker.data(), TICKER_SIZE), order.quantity,
        order.fillPrice, convert(order.state), convert(order.action));
    builder.Finish(msg);
    return builder.Release();
  }

  static DetachedBuffer serialize(const TickerPrice &price) {
    flatbuffers::FlatBufferBuilder builder;
    auto msg = gen::fbs::CreateTickerPrice(
        builder, builder.CreateString(price.ticker.data(), TICKER_SIZE), price.price);
    builder.Finish(msg);
    return builder.Release();
  }
};

} // namespace hft::serialization

#endif // HFT_COMMON_SERIALIZATION_FBSERIALIZER_HPP
