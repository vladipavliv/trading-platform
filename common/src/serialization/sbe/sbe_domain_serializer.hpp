/**
 * @author Vladimir Pavliv
 * @date 2025-08-09
 */

#ifndef HFT_COMMON_SERIALIZATION_SBESERIALIZER_HPP
#define HFT_COMMON_SERIALIZATION_SBESERIALIZER_HPP

#include "gen/sbe/cpp/hft_serialization_gen_sbe_domain/Char32.h"
#include "gen/sbe/cpp/hft_serialization_gen_sbe_domain/Char4.h"
#include "gen/sbe/cpp/hft_serialization_gen_sbe_domain/LoginRequest.h"
#include "gen/sbe/cpp/hft_serialization_gen_sbe_domain/LoginResponse.h"
#include "gen/sbe/cpp/hft_serialization_gen_sbe_domain/Message.h"
#include "gen/sbe/cpp/hft_serialization_gen_sbe_domain/MessageHeader.h"
#include "gen/sbe/cpp/hft_serialization_gen_sbe_domain/Order.h"
#include "gen/sbe/cpp/hft_serialization_gen_sbe_domain/OrderAction.h"
#include "gen/sbe/cpp/hft_serialization_gen_sbe_domain/OrderState.h"
#include "gen/sbe/cpp/hft_serialization_gen_sbe_domain/OrderStatus.h"
#include "gen/sbe/cpp/hft_serialization_gen_sbe_domain/TickerPrice.h"
#include "gen/sbe/cpp/hft_serialization_gen_sbe_domain/TokenBindRequest.h"

#include "concepts/busable.hpp"
#include "domain_types.hpp"
#include "sbe_converter.hpp"
#include "ticker.hpp"
#include "types.hpp"

namespace hft::serialization::sbe {

class DomainSerializer {
public:
  using SupportedTypes =
      std::tuple<LoginRequest, TokenBindRequest, LoginResponse, Order, OrderStatus, TickerPrice>;

  template <typename EventType>
  static constexpr bool Serializable = IsTypeInTuple<EventType, SupportedTypes>;

  template <Busable Consumer>
  static bool deserialize(const uint8_t *data, size_t size, Consumer &consumer) {
    using namespace hft::serialization::gen::sbe;

    domain::MessageHeader header(data, size);
    const size_t messageType = header.templateId();
    const size_t headerSize = header.encodedLength();
    const size_t messageSize = size - headerSize;

    switch (header.templateId()) {
    case domain::LoginRequest::sbeTemplateId(): {
      domain::LoginRequest msg(data + headerSize, messageSize);
      consumer.post(
          LoginRequest{msg.name().getChar32AsString(), msg.password().getChar32AsString()});
      return true;
    }
    case domain::TokenBindRequest::sbeTemplateId(): {
      domain::TokenBindRequest msg(data + headerSize, messageSize);
      consumer.post(TokenBindRequest{msg.token()});
      return true;
    }
    case domain::LoginResponse::sbeTemplateId(): {
      domain::LoginResponse msg(data + headerSize, messageSize);
      consumer.post(LoginResponse{msg.token(), msg.ok() == 1, msg.error_msg().getChar32AsString()});
      return true;
    }
    case domain::Order::sbeTemplateId(): {
      domain::Order msg(data + headerSize, messageSize);
      consumer.post(Order{msg.id(), msg.created(), makeTicker(msg.ticker().getChar4AsString()),
                          msg.quantity(), msg.price(), convert(msg.action())});
      return true;
    }
    case domain::OrderStatus::sbeTemplateId(): {
      domain::OrderStatus msg(data + headerSize, messageSize);
      consumer.post(OrderStatus{msg.order_id(), msg.timestamp(), msg.quantity(), msg.fill_price(),
                                convert(msg.state())});
      return true;
    }
    case domain::TickerPrice::sbeTemplateId(): {
      domain::TickerPrice msg(data + headerSize, messageSize);
      consumer.post(TickerPrice{makeTicker(msg.ticker().getChar4AsString()), msg.price()});
      return true;
    }
    default:
      LOG_ERROR("Unknown sbe message type");
      return false;
    }
  }

  static void serialize(CRef<LoginRequest> request, ByteBuffer &buffer) {
    using namespace hft::serialization::gen::sbe;
    buffer.resize(domain::MessageHeader::encodedLength() + domain::LoginRequest::SBE_BLOCK_LENGTH);
  }

  static void serialize(CRef<TokenBindRequest> request, ByteBuffer &buffer) {}

  static void serialize(CRef<LoginResponse> request, ByteBuffer &buffer) {}

  static void serialize(CRef<Order> request, ByteBuffer &buffer) {}

  static void serialize(CRef<OrderStatus> request, ByteBuffer &buffer) {}

  static void serialize(CRef<TickerPrice> request, ByteBuffer &buffer) {}

private:
};

} // namespace hft::serialization::sbe

#endif // HFT_COMMON_SERIALIZATION_SBESERIALIZER_HPP