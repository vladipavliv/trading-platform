/**
 * @author Vladimir Pavliv
 * @date 2025-02-13
 */

#ifndef HFT_COMMON_SERIALIZATION_FBSERIALIZER_HPP
#define HFT_COMMON_SERIALIZATION_FBSERIALIZER_HPP

#include <expected>

#include <spdlog/spdlog.h>

#include "bus/bus.hpp"
#include "constants.hpp"
#include "converter.hpp"
#include "gen/marketdata_generated.h"
#include "types/market_types.hpp"
#include "types/types.hpp"

namespace hft::serialization {

/**
 * @brief Flat buffers serializer
 * @details For simplicity posts deserialized messages directly to a bus
 * saves the trouble of type extraction, and performs slightly better
 * @todo improve later
 */
class FlatBuffersSerializer {
public:
  using Message = gen::fbs::Message;
  using MessageType = gen::fbs::MessageUnion;

  using BufferType = flatbuffers::DetachedBuffer;

  static bool deserialize(const uint8_t *data, size_t size, Bus &bus) {
    if (!flatbuffers::Verifier(data, size).VerifyBuffer<Message>()) {
      return false;
    }
    auto message = flatbuffers::GetRoot<Message>(data);
    if (message == nullptr) {
      return false;
    }
    switch (message->message_type()) {
    case MessageType::MessageUnion_Order: {
      auto orderMsg = message->message_as_Order();
      if (orderMsg == nullptr) {
        spdlog::error("Failed to extract Order");
        return false;
      }
      Order order{0,
                  orderMsg->id(),
                  fbStringToTicker(orderMsg->ticker()),
                  orderMsg->quantity(),
                  orderMsg->price(),
                  convert(orderMsg->action())};
      bus.marketBus.post(Span<Order>(&order, 1));
      return true;
    }
    case MessageType::MessageUnion_OrderStatus: {
      auto statusMsg = message->message_as_OrderStatus();
      if (statusMsg == nullptr) {
        spdlog::error("Failed to extract OrderStatus");
        return false;
      }
      OrderStatus status{0,
                         statusMsg->id(),
                         fbStringToTicker(statusMsg->ticker()),
                         statusMsg->quantity(),
                         statusMsg->fill_price(),
                         convert(statusMsg->state()),
                         convert(statusMsg->action())};
      bus.marketBus.post(Span<OrderStatus>(&status, 1));
      return true;
    }
    case MessageType::MessageUnion_TickerPrice: {
      auto priceMsg = message->message_as_TickerPrice();
      if (priceMsg == nullptr) {
        spdlog::error("Failed to extract TickerPrice");
        return false;
      }
      TickerPrice price{fbStringToTicker(priceMsg->ticker()), priceMsg->price()};
      bus.marketBus.post(Span<TickerPrice>(&price, 1));
      return true;
    }
    default:
      return false;
    }
  }

  static BufferType serialize(const Order &order) {
    flatbuffers::FlatBufferBuilder builder;
    auto msg = gen::fbs::CreateOrder(builder, order.id,
                                     builder.CreateString(order.ticker.data(), TICKER_SIZE),
                                     order.quantity, order.price, convert(order.action));
    builder.Finish(msg);
    return builder.Release();
  }

  static BufferType serialize(const OrderStatus &order) {
    flatbuffers::FlatBufferBuilder builder;
    auto msg = gen::fbs::CreateOrderStatus(
        builder, order.id, builder.CreateString(order.ticker.data(), TICKER_SIZE), order.quantity,
        order.fillPrice, convert(order.state), convert(order.action));
    builder.Finish(msg);
    return builder.Release();
  }

  static BufferType serialize(const TickerPrice &price) {
    flatbuffers::FlatBufferBuilder builder;
    auto msg = gen::fbs::CreateTickerPrice(
        builder, builder.CreateString(price.ticker.data(), TICKER_SIZE), price.price);
    builder.Finish(msg);
    return builder.Release();
  }
};

} // namespace hft::serialization

#endif // HFT_COMMON_SERIALIZATION_FBSERIALIZER_HPP
