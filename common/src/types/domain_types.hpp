/**
 * @author Vladimir Pavliv
 * @date 2025-02-13
 */

#ifndef HFT_COMMON_DOMAINTYPES_HPP
#define HFT_COMMON_DOMAINTYPES_HPP

#include <string>

#include "ticker.hpp"
#include "types.hpp"

namespace hft {

using OrderId = uint64_t;
using ClientId = uint64_t;
using Quantity = uint32_t;
using Price = uint32_t;

enum class OrderAction : uint8_t { Buy, Sell };

enum class OrderState : uint8_t { Accepted, Partial, Full };

struct LoginRequest {
  String name;
  String password;
};

struct TokenBindRequest {
  Token token;
};

struct LoginResponse {
  Token token{0};
  bool ok{false};
  String error{};
};

struct alignas(8) Order {
  OrderId id;
  Timestamp created;
  Ticker ticker;
  mutable Quantity quantity;
  Price price;
  OrderAction action;

  // TODO() Orders would be stored on the server as ServerOrder, so maybe not use padding here
  // with or without this padding it would push it beyond 32 bytes
  // TODO() Try separating hot and cold data on the server side
  char padding[3] = {0};

  inline void partialFill(Quantity amount) const {
    quantity = quantity < amount ? 0 : quantity - amount;
  }
};

struct alignas(8) OrderStatus {
  OrderId orderId;
  Timestamp fulfilled;
  Quantity quantity;
  Price fillPrice;
  OrderState state;
  char padding[7] = {0};
};

struct TickerPrice {
  Ticker ticker;
  Price price;
};

} // namespace hft

#endif // HFT_COMMON_DOMAINTYPES_HPP
