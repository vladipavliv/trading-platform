/**
 * @author Vladimir Pavliv
 * @date 2025-02-13
 */

#ifndef HFT_COMMON_DOMAINTYPES_HPP
#define HFT_COMMON_DOMAINTYPES_HPP

#include "ticker.hpp"
#include "types.hpp"

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
  auto operator<=>(CRef<LoginRequest>) const = default;
};

struct TokenBindRequest {
  Token token;
  auto operator<=>(CRef<TokenBindRequest>) const = default;
};

struct LoginResponse {
  Token token{0};
  bool ok{false};
  String error{};
  auto operator<=>(CRef<LoginResponse>) const = default;
};

struct Order {
  OrderId id;
  Timestamp created;
  Ticker ticker;
  mutable Quantity quantity;
  Price price;
  OrderAction action;

  char padding[3] = {0};

  void partialFill(Quantity amount) const { quantity = quantity < amount ? 0 : quantity - amount; }
  auto operator<=>(CRef<Order>) const = default;
};

struct OrderStatus {
  OrderId orderId;
  Timestamp timeStamp;
  Quantity quantity;
  Price fillPrice;
  OrderState state;
  char padding[7] = {0};
  auto operator<=>(CRef<OrderStatus>) const = default;
};

struct TickerPrice {
  Ticker ticker;
  Price price;
  auto operator<=>(CRef<TickerPrice>) const = default;
};

} // namespace hft

#endif // HFT_COMMON_DOMAINTYPES_HPP
