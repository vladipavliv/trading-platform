/**
 * @file
 * @brief
 *
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

class FlatBuffersSerializer {
public:
  using DetachedBuffer = flatbuffers::DetachedBuffer;

  template <typename MessageType>
  static std::enable_if_t<std::is_same<MessageType, Order>::value, Result<Order>>
  deserialize(const uint8_t *buffer, size_t size) {
    flatbuffers::Verifier verifier(buffer, size);
    if (!verifier.VerifyBuffer<gen::fbs::Order>()) {
      spdlog::error("Order verification failed.");
      return ErrorCode::Error;
    }
    auto orderMsg = flatbuffers::GetRoot<gen::fbs::Order>(buffer);
    return Order{orderMsg->id(), toString(orderMsg->ticker()), convert(orderMsg->action()),
                 orderMsg->quantity(), orderMsg->price()};
  }

  template <typename MessageType>
  static std::enable_if_t<std::is_same<MessageType, OrderStatus>::value, Result<OrderStatus>>
  deserialize(const uint8_t *buffer, size_t size) {
    flatbuffers::Verifier verifier(buffer, size);
    if (!verifier.VerifyBuffer<gen::fbs::OrderStatus>()) {
      spdlog::error("OrderStatus verification failed.");
      return ErrorCode::Error;
    }
    auto orderMsg = flatbuffers::GetRoot<gen::fbs::OrderStatus>(buffer);
    return OrderStatus{orderMsg->id(), convert(orderMsg->state()), orderMsg->quantity(),
                       orderMsg->fill_price()};
  }

  template <typename MessageType>
  static std::enable_if_t<std::is_same<MessageType, PriceUpdate>::value, Result<PriceUpdate>>
  deserialize(const uint8_t *buffer, size_t size) {
    flatbuffers::Verifier verifier(buffer, size);
    if (!verifier.VerifyBuffer<gen::fbs::PriceUpdate>()) {
      spdlog::error("PriceUpdate verification failed.");
      return ErrorCode::Error;
    }
    auto orderMsg = flatbuffers::GetRoot<gen::fbs::PriceUpdate>(buffer);
    return PriceUpdate{toString(orderMsg->ticker()), orderMsg->price()};
  }

  DetachedBuffer serialize(const Order &order) {
    flatbuffers::FlatBufferBuilder builder;

    auto msg = gen::fbs::CreateOrder(builder, order.id,
                                     builder.CreateString(order.ticker.data(), order.ticker.size()),
                                     convert(order.action), order.quantity, order.price);
    builder.Finish(msg);
    return builder.Release();
  }

  DetachedBuffer serialize(const OrderStatus &order) {
    flatbuffers::FlatBufferBuilder builder;

    auto msg = gen::fbs::CreateOrderStatus(builder, order.id, convert(order.state), order.quantity,
                                           order.fillPrice);
    builder.Finish(msg);
    return builder.Release();
  }

  DetachedBuffer serialize(const PriceUpdate &price) {
    flatbuffers::FlatBufferBuilder builder;

    auto msg = gen::fbs::CreatePriceUpdate(
        builder, builder.CreateString(price.ticker.data(), price.ticker.size()), price.price);
    builder.Finish(msg);
    return builder.Release();
  }
};

} // namespace hft::serialization

#endif // HFT_COMMON_SERIALIZATION_FBSERIALIZER_HPP