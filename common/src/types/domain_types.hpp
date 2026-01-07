/**
 * @author Vladimir Pavliv
 * @date 2025-02-13
 */

#ifndef HFT_COMMON_DOMAINTYPES_HPP
#define HFT_COMMON_DOMAINTYPES_HPP

#include "primitive_types.hpp"
#include "ticker.hpp"

namespace hft {

using OrderId = uint64_t;
using ClientId = uint64_t;
using Quantity = uint32_t;
using Price = uint32_t; // 1cent precision

enum class OrderAction : uint8_t { Buy, Sell };

enum class OrderState : uint8_t { Accepted, Rejected, Partial, Full };

struct LoginRequest {
  String name;
  String password;
  auto operator<=>(const LoginRequest &) const = default;
};

struct TokenBindRequest {
  Token token;
  auto operator<=>(const TokenBindRequest &) const = default;
};

struct LoginResponse {
  Token token{0};
  bool ok{false};
  String error{};
  auto operator<=>(const LoginResponse &) const = default;
};

struct Order {
  OrderId id;
  Timestamp created;
  Ticker ticker;
  Quantity quantity;
  Price price;
  OrderAction action;

  char padding[3] = {0};

  inline void partialFill(Quantity amount) { quantity = quantity < amount ? 0 : quantity - amount; }
  auto operator<=>(const Order &) const = default;
};

struct OrderStatus {
  OrderId orderId;
  Timestamp timeStamp;
  Quantity quantity;
  Price fillPrice;
  OrderState state;
  char padding[7] = {0};
  auto operator<=>(const OrderStatus &) const = default;
};

struct TickerPrice {
  Ticker ticker;
  Price price;
  auto operator<=>(const TickerPrice &) const = default;
};

inline String toString(const LoginRequest &msg) {
  return std::format("LoginRequest {} {}", msg.name, msg.password);
}

inline String toString(const TokenBindRequest &msg) {
  return std::format("TokenBindRequest {}", msg.token);
}

inline String toString(const LoginResponse &msg) {
  return std::format("LoginResponse {} {} {}", msg.token, msg.ok, msg.error);
}

inline String toString(const OrderState &state) {
  switch (state) {
  case OrderState::Accepted:
    return "Accepted";
  case OrderState::Rejected:
    return "Rejected";
  case OrderState::Partial:
    return "Partial";
  case OrderState::Full:
    return "Full";
  default:
    return "Unknown";
  }
}

inline String toString(const OrderAction &state) {
  switch (state) {
  case OrderAction::Buy:
    return "Buy";
  case OrderAction::Sell:
    return "Sell";
  default:
    return "Unknown";
  }
}

inline String toString(const Order &o) {
  return std::format("Order: id={} created={} ticker={} qty={} price={} action={}", o.id, o.created,
                     std::string_view(o.ticker.data(), TICKER_SIZE), o.quantity, o.price,
                     (o.action == OrderAction::Buy ? "Buy" : "Sell"));
}

inline String toString(const OrderStatus &status) {
  return std::format("OrderStatus: id={} ts={} qty={} fill_px={} state={}", status.orderId,
                     status.timeStamp, status.quantity, status.fillPrice, toString(status.state));
}

inline String toString(const TickerPrice &price) {
  // Use string_view to avoid allocation for the ticker data
  return std::format("{}: ${}", StringView(price.ticker.data(), TICKER_SIZE), price.price);
}

} // namespace hft

#endif // HFT_COMMON_DOMAINTYPES_HPP
