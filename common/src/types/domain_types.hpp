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

/**
 * @todo On the server side each OrderBook processes single ticker
 * Also in the current implementation separate container is used for bids and asks
 * So ticker and action could be thrown away to reduce memory footprint
 * Would complicate things a little bit, but it would help keeping order under 32 bytes
 * Which might improve performance even further having 2 orders in a single cache line
 */
struct alignas(8) Order {
  OrderId id;
  Timestamp created;
  Ticker ticker;
  mutable Quantity quantity;
  Price price;
  OrderAction action;

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
