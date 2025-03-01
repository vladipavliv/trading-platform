/**
 * @author Vladimir Pavliv
 * @date 2025-02-13
 */

#ifndef HFT_COMMON_SERIALIZATION_FBSERIALIZER_HPP
#define HFT_COMMON_SERIALIZATION_FBSERIALIZER_HPP

#include <spdlog/spdlog.h>

#include "converter.hpp"
#include "gen/marketdata_generated.h"
#include "pool/object_pool.hpp"
#include "types/market_types.hpp"
#include "types/result.hpp"
#include "types/types.hpp"

namespace hft::serialization {

class FlatBuffersSerializer {
public:
  using DetachedBuffer = flatbuffers::DetachedBuffer;

  template <typename MessageType>
  static std::enable_if_t<std::is_same<MessageType, PoolPtr<Order>>::value, PoolPtr<Order>>
  deserialize(const uint8_t *buffer, size_t size) {
    flatbuffers::Verifier verifier(buffer, size);
    if (!verifier.VerifyBuffer<gen::fbs::Order>()) {
      spdlog::error("Order verification failed");
      return PoolPtr<Order>();
    }
    auto msg = flatbuffers::GetRoot<gen::fbs::Order>(buffer);
    auto orderPtr = PoolPtr<Order>::create();
    orderPtr->id = msg->id();
    orderPtr->ticker = fbStringToTicker(msg->ticker());
    orderPtr->quantity = msg->quantity();
    orderPtr->price = msg->price();
    orderPtr->action = convert(msg->action());
    return orderPtr;
  }

  template <typename MessageType>
  static std::enable_if_t<std::is_same<MessageType, PoolPtr<OrderStatus>>::value,
                          PoolPtr<OrderStatus>>
  deserialize(const uint8_t *buffer, size_t size) {
    flatbuffers::Verifier verifier(buffer, size);
    if (!verifier.VerifyBuffer<gen::fbs::OrderStatus>()) {
      spdlog::error("OrderStatus verification failed");
      return PoolPtr<OrderStatus>();
    }
    auto orderMsg = flatbuffers::GetRoot<gen::fbs::OrderStatus>(buffer);
    auto statusPtr = PoolPtr<OrderStatus>::create();
    statusPtr->id = orderMsg->id();
    statusPtr->ticker = fbStringToTicker(orderMsg->ticker());
    statusPtr->quantity = orderMsg->quantity();
    statusPtr->fillPrice = orderMsg->fill_price();
    statusPtr->state = convert(orderMsg->state());
    statusPtr->action = convert(orderMsg->action());
    return statusPtr;
  }

  template <typename MessageType>
  static std::enable_if_t<std::is_same<MessageType, PoolPtr<TickerPrice>>::value,
                          PoolPtr<TickerPrice>>
  deserialize(const uint8_t *buffer, size_t size) {
    flatbuffers::Verifier verifier(buffer, size);
    if (!verifier.VerifyBuffer<gen::fbs::TickerPrice>()) {
      spdlog::error("TickerPrice verification failed");
      return PoolPtr<TickerPrice>();
    }
    auto orderMsg = flatbuffers::GetRoot<gen::fbs::TickerPrice>(buffer);
    auto pricePtr = PoolPtr<TickerPrice>::create();
    pricePtr->ticker = fbStringToTicker(orderMsg->ticker());
    pricePtr->price = orderMsg->price();
    return pricePtr;
  }

  static DetachedBuffer serialize(const PoolPtr<Order> &order) {
    flatbuffers::FlatBufferBuilder builder;
    auto msg = gen::fbs::CreateOrder(builder, order->id,
                                     builder.CreateString(order->ticker.data(), TICKER_SIZE),
                                     order->quantity, order->price, convert(order->action));
    builder.Finish(msg);
    return builder.Release();
  }

  static DetachedBuffer serialize(const PoolPtr<OrderStatus> &order) {
    flatbuffers::FlatBufferBuilder builder;
    auto msg = gen::fbs::CreateOrderStatus(
        builder, order->id, builder.CreateString(order->ticker.data(), TICKER_SIZE),
        order->quantity, order->fillPrice, convert(order->state), convert(order->action));
    builder.Finish(msg);
    return builder.Release();
  }

  static DetachedBuffer serialize(const PoolPtr<TickerPrice> &price) {
    flatbuffers::FlatBufferBuilder builder;
    auto msg = gen::fbs::CreateTickerPrice(
        builder, builder.CreateString(price->ticker.data(), TICKER_SIZE), price->price);
    builder.Finish(msg);
    return builder.Release();
  }
};

} // namespace hft::serialization

#endif // HFT_COMMON_SERIALIZATION_FBSERIALIZER_HPP
