/**
 * @author Vladimir Pavliv
 * @date 2025-02-13
 */

#ifndef HFT_COMMON_SERIALIZATION_FBSSERIALIZER_HPP
#define HFT_COMMON_SERIALIZATION_FBSSERIALIZER_HPP

#include "bus/busable.hpp"
#include "constants.hpp"
#include "domain_types.hpp"
#include "fbs_converter.hpp"
#include "gen/fbs/cpp/domain_messages_generated.h"
#include "types.hpp"
#include "utils/string_utils.hpp"

namespace hft::serialization::fbs {

/**
 * @brief Flat buffers serializer
 */
class FbsDomainSerializer {
public:
  using Message = gen::fbs::domain::Message;
  using MessageType = gen::fbs::domain::MessageUnion;
  using BufferType = flatbuffers::DetachedBuffer;

  using SupportedTypes =
      std::tuple<LoginRequest, TokenBindRequest, LoginResponse, Order, OrderStatus, TickerPrice>;

  template <typename EventType>
  static constexpr bool Serializable = IsTypeInTuple<EventType, SupportedTypes>;

  template <Busable Consumer>
  static size_t deserialize(const uint8_t *data, size_t size, Consumer &consumer) {
    if (!flatbuffers::Verifier(data, size).VerifyBuffer<Message>()) {
      LOG_ERROR("Failed to verify Buffer {}", size);
      return 0;
    }
    const auto message = flatbuffers::GetRoot<Message>(data);
    if (message == nullptr) {
      LOG_ERROR("Failed to extract Message {}", size);
      return 0;
    }
    const auto type = message->message_type();
    switch (type) {
    case MessageType::MessageUnion_LoginRequest: {
      const auto msg = message->message_as_LoginRequest();
      if (msg == nullptr) {
        LOG_ERROR("Failed to extract LoginRequest");
        return 0;
      }
      const LoginRequest request{fbStringToString(msg->name()), fbStringToString(msg->password())};
      consumer.post(request);
      break;
    }
    case MessageType::MessageUnion_TokenBindRequest: {
      const auto msg = message->message_as_TokenBindRequest();
      if (msg == nullptr) {
        LOG_ERROR("Failed to extract TokenBindRequest");
        return 0;
      }
      const TokenBindRequest response{msg->token()};
      consumer.post(response);
      break;
    }
    case MessageType::MessageUnion_LoginResponse: {
      const auto msg = message->message_as_LoginResponse();
      if (msg == nullptr) {
        LOG_ERROR("Failed to extract LoginResponse");
        return 0;
      }
      const LoginResponse response{msg->token(), msg->ok(), fbStringToString(msg->error())};
      consumer.post(response);
      break;
    }
    case MessageType::MessageUnion_Order: {
      const auto orderMsg = message->message_as_Order();
      if (orderMsg == nullptr) {
        LOG_ERROR("Failed to extract Order");
        return 0;
      }
      const Order order{
          orderMsg->id(),       orderMsg->created(), fbStringToTicker(orderMsg->ticker()),
          orderMsg->quantity(), orderMsg->price(),   convert(orderMsg->action())};
      consumer.post(order);
      break;
    }
    case MessageType::MessageUnion_OrderStatus: {
      const auto statusMsg = message->message_as_OrderStatus();
      if (statusMsg == nullptr) {
        LOG_ERROR("Failed to extract OrderStatus");
        return 0;
      }
      const OrderStatus status{statusMsg->order_id(), statusMsg->timestamp(), statusMsg->quantity(),
                               statusMsg->fill_price(), convert(statusMsg->state())};
      consumer.post(status);
      break;
    }
    case MessageType::MessageUnion_TickerPrice: {
      const auto priceMsg = message->message_as_TickerPrice();
      if (priceMsg == nullptr) {
        LOG_ERROR("Failed to extract TickerPrice");
        return 0;
      }
      const TickerPrice price{fbStringToTicker(priceMsg->ticker()), priceMsg->price()};
      consumer.post(price);
      break;
    }
    default:
      LOG_ERROR("Unknown message type {}", static_cast<uint8_t>(type));
      return 0;
    }
    return size;
  }

  static size_t serialize(CRef<LoginRequest> request, uint8_t *buffer) {
    using namespace gen::fbs::domain;
    flatbuffers::FlatBufferBuilder builder;
    const auto msg = CreateLoginRequest(
        builder, builder.CreateString(request.name.c_str(), request.name.length()),
        builder.CreateString(request.password.c_str(), request.password.length()));
    builder.Finish(CreateMessage(builder, MessageUnion_LoginRequest, msg.Union()));
    const auto serializedMsg = builder.Release();

    std::memcpy(buffer, serializedMsg.data(), serializedMsg.size());
    return serializedMsg.size();
  }

  static size_t serialize(CRef<TokenBindRequest> request, uint8_t *buffer) {
    using namespace gen::fbs::domain;
    flatbuffers::FlatBufferBuilder builder;
    const auto msg = CreateTokenBindRequest(builder, request.token);
    builder.Finish(CreateMessage(builder, MessageUnion_TokenBindRequest, msg.Union()));
    const auto serializedMsg = builder.Release();

    std::memcpy(buffer, serializedMsg.data(), serializedMsg.size());
    return serializedMsg.size();
  }

  static size_t serialize(CRef<LoginResponse> response, uint8_t *buffer) {
    using namespace gen::fbs::domain;
    flatbuffers::FlatBufferBuilder builder;
    const auto msg =
        CreateLoginResponse(builder, response.token, response.ok,
                            builder.CreateString(response.error.c_str(), response.error.length()));
    builder.Finish(CreateMessage(builder, MessageUnion_LoginResponse, msg.Union()));
    const auto serializedMsg = builder.Release();

    std::memcpy(buffer, serializedMsg.data(), serializedMsg.size());
    return serializedMsg.size();
  }

  static size_t serialize(CRef<Order> order, uint8_t *buffer) {
    using namespace gen::fbs::domain;
    flatbuffers::FlatBufferBuilder builder;
    const auto msg = CreateOrder(builder, order.id, order.created,
                                 builder.CreateString(order.ticker.data(), TICKER_SIZE),
                                 order.quantity, order.price, convert(order.action));
    builder.Finish(CreateMessage(builder, MessageUnion_Order, msg.Union()));
    const auto serializedMsg = builder.Release();

    std::memcpy(buffer, serializedMsg.data(), serializedMsg.size());
    return serializedMsg.size();
  }

  static size_t serialize(CRef<OrderStatus> status, uint8_t *buffer) {
    using namespace gen::fbs::domain;
    flatbuffers::FlatBufferBuilder builder;
    const auto msg = CreateOrderStatus(builder, status.orderId, status.timeStamp, status.quantity,
                                       status.fillPrice, convert(status.state));
    builder.Finish(CreateMessage(builder, MessageUnion_OrderStatus, msg.Union()));
    const auto serializedMsg = builder.Release();

    std::memcpy(buffer, serializedMsg.data(), serializedMsg.size());
    return serializedMsg.size();
  }

  static size_t serialize(CRef<TickerPrice> price, uint8_t *buffer) {
    using namespace gen::fbs::domain;
    flatbuffers::FlatBufferBuilder builder;
    const auto msg = CreateTickerPrice(
        builder, builder.CreateString(price.ticker.data(), TICKER_SIZE), price.price);
    builder.Finish(CreateMessage(builder, MessageUnion_TickerPrice, msg.Union()));
    const auto serializedMsg = builder.Release();

    std::memcpy(buffer, serializedMsg.data(), serializedMsg.size());
    return serializedMsg.size();
  }
};

} // namespace hft::serialization::fbs

#endif // HFT_COMMON_SERIALIZATION_FBSSERIALIZER_HPP
