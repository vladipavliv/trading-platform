/**
 * @author Vladimir Pavliv
 * @date 2025-02-13
 */

#ifndef HFT_COMMON_DOMAINTYPES_HPP
#define HFT_COMMON_DOMAINTYPES_HPP

#include "primitive_types.hpp"
#include "ticker.hpp"

namespace hft {

using OrderId = uint32_t;
using ClientId = uint32_t;
using Quantity = uint32_t;
using Price = uint32_t;

enum class OrderAction : uint8_t {
  Dummy = 0,
  Buy = 1 << 0,
  Sell = 1 << 1,
  Modify = 1 << 2,
  Cancel = 1 << 3
};

enum class OrderState : uint8_t { Accepted, Rejected, Cancelled, Partial, Full };

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
  auto operator<=>(const Order &) const = default;
};

struct OrderStatus {
  OrderId orderId;
  Timestamp timeStamp;
  Quantity quantity;
  Price fillPrice;
  OrderState state;
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
  case OrderState::Cancelled:
    return "Cancelled";
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
  case OrderAction::Dummy:
    return "Dummy";
  case OrderAction::Buy:
    return "Buy";
  case OrderAction::Sell:
    return "Sell";
  case OrderAction::Modify:
    return "Modify";
  case OrderAction::Cancel:
    return "Cancel";
  default:
    return "Unknown";
  }
}

inline String toString(const Order &o) {
  return std::format("Order: Id={} Created={} Ticker={} Qty={} Price={} Action={}", o.id, o.created,
                     std::string_view(o.ticker.data(), TICKER_SIZE), o.quantity, o.price,
                     (o.action == OrderAction::Buy ? "Buy" : "Sell"));
}

inline String toString(const OrderStatus &status) {
  return std::format("OrderStatus: Id={} Ts={} Qty={} FillPrice={} State={}", status.orderId,
                     status.timeStamp, status.quantity, status.fillPrice, toString(status.state));
}

inline String toString(const TickerPrice &price) {
  return std::format("{}: ${}", StringView(price.ticker.data(), TICKER_SIZE), price.price);
}

} // namespace hft

#endif // HFT_COMMON_DOMAINTYPES_HPP
