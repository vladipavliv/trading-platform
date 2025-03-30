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
#include "utils/string_utils.hpp"

namespace hft::serialization {

/**
 * @brief Flat buffers serializer
 * @details For simplicity posts deserialized messages directly to a bus
 * saves the trouble of type extraction, and performs slightly better
 * @todo improve
 */
class FlatBuffersSerializer {
public:
  using Message = gen::fbs::Message;
  using MessageType = gen::fbs::MessageUnion;
  using BufferType = flatbuffers::DetachedBuffer;

  using SupportedTypes = std::tuple<LoginRequest, LoginResponse, Order, OrderStatus, TickerPrice>;

  template <typename EventType>
  static constexpr bool Serializable = IsTypeInTuple<EventType, SupportedTypes>;

  template <typename Consumer>
  static bool deserialize(const uint8_t *data, size_t size, Consumer &consumer) {
    if (!flatbuffers::Verifier(data, size).VerifyBuffer<Message>()) {
      spdlog::error("Failed to verify Buffer");
      return false;
    }
    auto message = flatbuffers::GetRoot<Message>(data);
    if (message == nullptr) {
      spdlog::error("Failed to extract Message");
      return false;
    }
    const auto type = message->message_type();
    switch (type) {
    case MessageType::MessageUnion_LoginRequest: {
      auto msg = message->message_as_LoginRequest();
      if (msg == nullptr) {
        spdlog::error("Failed to extract LoginRequest");
        return false;
      }
      LoginRequest request{fbStringToString(msg->name()), fbStringToString(msg->password())};
      spdlog::trace("Deserialized {}", [&request]() { return utils::toString(request); }());
      consumer.template post<LoginRequest>(request);
      return true;
    }
    case MessageType::MessageUnion_LoginResponse: {
      auto msg = message->message_as_LoginResponse();
      if (msg == nullptr) {
        spdlog::error("Failed to extract LoginResponse");
        return false;
      }
      LoginResponse response{fbStringToString(msg->token()), msg->success()};
      spdlog::trace("Deserialized {}", [&response]() { return utils::toString(response); }());
      consumer.template post<LoginResponse>(response);
      return true;
    }
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
      spdlog::trace("Deserialized {}", [&order]() { return utils::toString(order); }());
      consumer.template post<Order>(Span<Order>(&order, 1));
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
      spdlog::trace("Deserialized {}", [&status]() { return utils::toString(status); }());
      consumer.template post<OrderStatus>(Span<OrderStatus>(&status, 1));
      return true;
    }
    case MessageType::MessageUnion_TickerPrice: {
      auto priceMsg = message->message_as_TickerPrice();
      if (priceMsg == nullptr) {
        spdlog::error("Failed to extract TickerPrice");
        return false;
      }
      TickerPrice price{fbStringToTicker(priceMsg->ticker()), priceMsg->price()};
      spdlog::trace("Deserialized {}", [&price]() { return utils::toString(price); }());
      consumer.template post<TickerPrice>(Span<TickerPrice>(&price, 1));
      return true;
    }
    default:
      break;
    }
    spdlog::error("Unknown message type {}", static_cast<uint8_t>(type));
    return false;
  }

  static BufferType serialize(const LoginRequest &request) {
    spdlog::trace("Serializing {}", [&request]() { return utils::toString(request); }());
    using namespace gen::fbs;
    flatbuffers::FlatBufferBuilder builder;
    auto msg = CreateLoginRequest(
        builder, builder.CreateString(request.name.c_str(), request.name.length()),
        builder.CreateString(request.password.c_str(), request.password.length()));
    builder.Finish(CreateMessage(builder, MessageUnion_LoginRequest, msg.Union()));
    return builder.Release();
  }

  static BufferType serialize(const LoginResponse &response) {
    spdlog::trace("Serializing {}", [&response]() { return utils::toString(response); }());
    using namespace gen::fbs;
    flatbuffers::FlatBufferBuilder builder;
    auto msg =
        CreateLoginResponse(builder, response.success,
                            builder.CreateString(response.token.c_str(), response.token.length()));
    builder.Finish(CreateMessage(builder, MessageUnion_LoginResponse, msg.Union()));
    return builder.Release();
  }

  static BufferType serialize(const Order &order) {
    spdlog::trace("Serializing {}", [&order]() { return utils::toString(order); }());
    using namespace gen::fbs;
    flatbuffers::FlatBufferBuilder builder;
    auto msg =
        CreateOrder(builder, order.id, builder.CreateString(order.ticker.data(), TICKER_SIZE),
                    order.quantity, order.price, convert(order.action));
    builder.Finish(CreateMessage(builder, MessageUnion_Order, msg.Union()));
    return builder.Release();
  }

  static BufferType serialize(const OrderStatus &order) {
    spdlog::trace("Serializing {}", [&order]() { return utils::toString(order); }());
    using namespace gen::fbs;
    flatbuffers::FlatBufferBuilder builder;
    auto msg = CreateOrderStatus(
        builder, order.id, builder.CreateString(order.ticker.data(), TICKER_SIZE), order.quantity,
        order.fillPrice, convert(order.state), convert(order.action));
    builder.Finish(CreateMessage(builder, MessageUnion_OrderStatus, msg.Union()));
    return builder.Release();
  }

  static BufferType serialize(const TickerPrice &price) {
    spdlog::trace("Serializing {}", [&price]() { return utils::toString(price); }());
    using namespace gen::fbs;
    flatbuffers::FlatBufferBuilder builder;
    auto msg = gen::fbs::CreateTickerPrice(
        builder, builder.CreateString(price.ticker.data(), TICKER_SIZE), price.price);
    builder.Finish(CreateMessage(builder, MessageUnion_TickerPrice, msg.Union()));
    return builder.Release();
  }
};

} // namespace hft::serialization

#endif // HFT_COMMON_SERIALIZATION_FBSERIALIZER_HPP
