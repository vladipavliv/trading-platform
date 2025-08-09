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

class SbeDomainSerializer {
public:
  using SupportedTypes =
      std::tuple<LoginRequest, TokenBindRequest, LoginResponse, Order, OrderStatus, TickerPrice>;

  template <typename EventType>
  static constexpr bool Serializable = IsTypeInTuple<EventType, SupportedTypes>;

  template <Busable Consumer>
  static bool deserialize(const uint8_t *buffer, size_t size, Consumer &consumer) {
    using namespace hft::serialization::gen::sbe;
    char *data = reinterpret_cast<char *>(const_cast<uint8_t *>(buffer));

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

  static void serialize(CRef<LoginRequest> r, ByteBuffer &buffer) {
    using namespace hft::serialization::gen::sbe;
    buffer.resize(domain::LoginRequest::sbeBlockAndHeaderLength());

    domain::LoginRequest msg;
    msg.wrapAndApplyHeader(reinterpret_cast<char *>(buffer.data()), 0, buffer.size());
    msg.name().putChar32(r.name);
    msg.password().putChar32(r.password);
  }

  static void serialize(CRef<TokenBindRequest> r, ByteBuffer &buffer) {
    using namespace hft::serialization::gen::sbe;
    buffer.resize(domain::TokenBindRequest::sbeBlockAndHeaderLength());

    domain::TokenBindRequest msg;
    msg.wrapAndApplyHeader(reinterpret_cast<char *>(buffer.data()), 0, buffer.size());
    msg.token(r.token);
  }

  static void serialize(CRef<LoginResponse> r, ByteBuffer &buffer) {
    using namespace hft::serialization::gen::sbe;
    buffer.resize(domain::LoginResponse::sbeBlockAndHeaderLength());

    domain::LoginResponse msg;
    msg.wrapAndApplyHeader(reinterpret_cast<char *>(buffer.data()), 0, buffer.size());
    msg.token(r.token).ok(r.ok).error_msg().putChar32(r.error);
  }

  static void serialize(CRef<Order> r, ByteBuffer &buffer) {
    using namespace hft::serialization::gen::sbe;
    buffer.resize(domain::Order::sbeBlockAndHeaderLength());

    domain::Order msg;
    msg.wrapAndApplyHeader(reinterpret_cast<char *>(buffer.data()), 0, buffer.size());
    msg.id(r.id).created(r.created).ticker().putChar4(r.ticker.data());
    msg.quantity(r.quantity).price(r.price).action(convert(r.action));
  }

  static void serialize(CRef<OrderStatus> r, ByteBuffer &buffer) {
    using namespace hft::serialization::gen::sbe;
    buffer.resize(domain::OrderStatus::sbeBlockAndHeaderLength());

    domain::OrderStatus msg;
    msg.wrapAndApplyHeader(reinterpret_cast<char *>(buffer.data()), 0, buffer.size());
    msg.order_id(r.orderId).timestamp(r.timeStamp).quantity(r.quantity);
    msg.fill_price(r.fillPrice).state(convert(r.state));
  }

  static void serialize(CRef<TickerPrice> r, ByteBuffer &buffer) {
    using namespace hft::serialization::gen::sbe;
    buffer.resize(domain::TickerPrice::sbeBlockAndHeaderLength());

    domain::TickerPrice msg;
    msg.wrapAndApplyHeader(reinterpret_cast<char *>(buffer.data()), 0, buffer.size());
    msg.ticker().putChar4(r.ticker.data());
    msg.price(r.price);
  }

private:
};

} // namespace hft::serialization::sbe

#endif // HFT_COMMON_SERIALIZATION_SBESERIALIZER_HPP