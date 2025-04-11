/**
 * @author Vladimir Pavliv
 * @date 2025-02-13
 */

#ifndef HFT_COMMON_MARKET_TYPES_HPP
#define HFT_COMMON_MARKET_TYPES_HPP

#include <algorithm>
#include <cstring>
#include <string_view>

#include "types.hpp"

namespace hft {

using OrderId = uint64_t;
using TraderId = uint64_t;
using Quantity = uint32_t;
using Price = uint32_t;

constexpr size_t TICKER_SIZE = 4;
using Ticker = std::array<char, TICKER_SIZE>;

struct TickerHash {
  std::size_t operator()(const Ticker &t) const {
    return std::hash<std::string_view>{}(std::string_view(t.data(), t.size()));
  }
};

struct CredentialsLoginRequest {
  mutable SocketId socketId; // Server side
  const String name;
  const String password; // TODO() encrypt

  inline void setSocketId(SocketId id) const { socketId = id; }
};

struct TokenLoginRequest {
  mutable SocketId socketId; // Server side
  const SessionToken token;

  inline void setSocketId(SocketId id) const { socketId = id; }
};

struct LoginResponse {
  SocketId socketId{0}; // Server side
  TraderId traderId{0}; // Server side
  mutable SessionToken token;
  bool success;

  inline void setToken(SessionToken tok) const { token = tok; }
};

enum class OrderAction : uint8_t { Buy, Sell };

enum class OrderState : uint8_t { Accepted, Partial, Full };

struct alignas(8) Order {
  mutable TraderId traderId; // Server side
  mutable SessionToken token;
  OrderId id;
  Timestamp timestamp;
  Ticker ticker;
  mutable Quantity quantity;
  Price price;
  OrderAction action;

  inline void setTraderId(TraderId id) const { traderId = id; }
  inline void setToken(SessionToken tok) const { token = tok; }
  inline void reduceQuantity(Quantity amount) const {
    quantity = quantity < amount ? 0 : quantity - amount;
  }
};

struct alignas(8) OrderStatus {
  mutable TraderId traderId; // Server side
  SessionToken token;        // Server side
  OrderId orderId;
  Timestamp fulfilled;
  Ticker ticker;
  Quantity quantity;
  Price fillPrice;
  OrderState state;

  inline void setTraderId(TraderId id) const { traderId = id; }
};

struct TickerPrice {
  const Ticker ticker;
  const Price price;
};

} // namespace hft

#endif // HFT_COMMON_MARKET_TYPES_HPP
