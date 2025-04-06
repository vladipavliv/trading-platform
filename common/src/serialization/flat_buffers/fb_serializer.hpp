/**
 * @author Vladimir Pavliv
 * @date 2025-02-13
 */

#ifndef HFT_COMMON_SERIALIZATION_FBSERIALIZER_HPP
#define HFT_COMMON_SERIALIZATION_FBSERIALIZER_HPP

#include <expected>

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

  using SupportedTypes = std::tuple<CredentialsLoginRequest, TokenLoginRequest, LoginResponse,
                                    Order, OrderStatus, TickerPrice>;

  template <typename EventType>
  static constexpr bool Serializable = IsTypeInTuple<EventType, SupportedTypes>;

  template <typename Consumer>
  static bool deserialize(const uint8_t *data, size_t size, Consumer &&consumer) {
    if (!flatbuffers::Verifier(data, size).VerifyBuffer<Message>()) {
      LOG_ERROR("Failed to verify Buffer");
      return false;
    }
    auto message = flatbuffers::GetRoot<Message>(data);
    if (message == nullptr) {
      LOG_ERROR("Failed to extract Message");
      return false;
    }
    const auto type = message->message_type();
    switch (type) {
    case MessageType::MessageUnion_CredentialsLoginRequest: {
      auto msg = message->message_as_CredentialsLoginRequest();
      if (msg == nullptr) {
        LOG_ERROR("Failed to extract CredentialsLoginRequest");
        return false;
      }
      const CredentialsLoginRequest request{0, fbStringToString(msg->name()),
                                            fbStringToString(msg->password())};
      LOG_DEBUG("Deserialized {}", utils::toString(request));
      consumer.post(request);
      return true;
    }
    case MessageType::MessageUnion_TokenLoginRequest: {
      auto msg = message->message_as_TokenLoginRequest();
      if (msg == nullptr) {
        LOG_ERROR("Failed to extract TokenLoginRequest");
        return false;
      }
      const TokenLoginRequest response{0, msg->token()};
      LOG_DEBUG("Deserialized {}", utils::toString(response));
      consumer.post(response);
      return true;
    }
    case MessageType::MessageUnion_LoginResponse: {
      auto msg = message->message_as_LoginResponse();
      if (msg == nullptr) {
        LOG_ERROR("Failed to extract LoginResponse");
        return false;
      }
      const LoginResponse response{0, 0, msg->token(), msg->success()};
      LOG_DEBUG("Deserialized {}", utils::toString(response));
      consumer.post(response);
      return true;
    }
    case MessageType::MessageUnion_Order: {
      auto orderMsg = message->message_as_Order();
      if (orderMsg == nullptr) {
        LOG_ERROR("Failed to extract Order");
        return false;
      }
      const Order order{0,
                        orderMsg->token(),
                        orderMsg->id(),
                        orderMsg->timestamp(),
                        fbStringToTicker(orderMsg->ticker()),
                        orderMsg->quantity(),
                        orderMsg->price(),
                        convert(orderMsg->action())};
      LOG_DEBUG("Deserialized {}", utils::toString(order));
      consumer.post(order);
      return true;
    }
    case MessageType::MessageUnion_OrderStatus: {
      auto statusMsg = message->message_as_OrderStatus();
      if (statusMsg == nullptr) {
        LOG_ERROR("Failed to extract OrderStatus");
        return false;
      }
      const OrderStatus status{0,
                               0,
                               statusMsg->order_id(),
                               statusMsg->timestamp(),
                               fbStringToTicker(statusMsg->ticker()),
                               statusMsg->quantity(),
                               statusMsg->fill_price(),
                               convert(statusMsg->state()),
                               convert(statusMsg->action())};
      LOG_DEBUG("Deserialized {}", utils::toString(status));
      consumer.post(status);
      return true;
    }
    case MessageType::MessageUnion_TickerPrice: {
      auto priceMsg = message->message_as_TickerPrice();
      if (priceMsg == nullptr) {
        LOG_ERROR("Failed to extract TickerPrice");
        return false;
      }
      const TickerPrice price{fbStringToTicker(priceMsg->ticker()), priceMsg->price()};
      LOG_DEBUG("Deserialized {}", utils::toString(price));
      consumer.post(price);
      return true;
    }
    default:
      break;
    }
    LOG_ERROR("Unknown message type {}", static_cast<uint8_t>(type));
    return false;
  }

  static BufferType serialize(const CredentialsLoginRequest &request) {
    LOG_DEBUG("Serializing {}", utils::toString(request));
    using namespace gen::fbs;
    flatbuffers::FlatBufferBuilder builder;
    auto msg = CreateCredentialsLoginRequest(
        builder, builder.CreateString(request.name.c_str(), request.name.length()),
        builder.CreateString(request.password.c_str(), request.password.length()));
    builder.Finish(CreateMessage(builder, MessageUnion_CredentialsLoginRequest, msg.Union()));
    return builder.Release();
  }

  static BufferType serialize(const TokenLoginRequest &request) {
    LOG_DEBUG("Serializing {}", utils::toString(request));
    using namespace gen::fbs;
    flatbuffers::FlatBufferBuilder builder;
    auto msg = CreateTokenLoginRequest(builder, request.token);
    builder.Finish(CreateMessage(builder, MessageUnion_TokenLoginRequest, msg.Union()));
    return builder.Release();
  }

  static BufferType serialize(const LoginResponse &response) {
    LOG_DEBUG("Serializing {}", utils::toString(response));
    using namespace gen::fbs;
    flatbuffers::FlatBufferBuilder builder;
    auto msg = CreateLoginResponse(builder, response.token, response.success);
    builder.Finish(CreateMessage(builder, MessageUnion_LoginResponse, msg.Union()));
    return builder.Release();
  }

  static BufferType serialize(const Order &order) {
    LOG_DEBUG("Serializing {}", utils::toString(order));
    using namespace gen::fbs;
    flatbuffers::FlatBufferBuilder builder;
    auto msg = CreateOrder(builder, order.token, order.id, order.timestamp,
                           builder.CreateString(order.ticker.data(), TICKER_SIZE), order.quantity,
                           order.price, convert(order.action));
    builder.Finish(CreateMessage(builder, MessageUnion_Order, msg.Union()));
    return builder.Release();
  }

  static BufferType serialize(const OrderStatus &status) {
    LOG_DEBUG("Serializing {}", utils::toString(status));
    using namespace gen::fbs;
    flatbuffers::FlatBufferBuilder builder;
    auto msg =
        CreateOrderStatus(builder, status.orderId, status.timestamp,
                          builder.CreateString(status.ticker.data(), TICKER_SIZE), status.quantity,
                          status.fillPrice, convert(status.state), convert(status.action));
    builder.Finish(CreateMessage(builder, MessageUnion_OrderStatus, msg.Union()));
    return builder.Release();
  }

  static BufferType serialize(const TickerPrice &price) {
    LOG_DEBUG("Serializing {}", utils::toString(price));
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
