/**
 * @author Vladimir Pavliv
 * @date 2025-02-13
 */

#ifndef HFT_COMMON_MARKET_TYPES_HPP
#define HFT_COMMON_MARKET_TYPES_HPP

#include <algorithm>
#include <cstring>
#include <string_view>

#include "pool/object_pool.hpp"
#include "types.hpp"

namespace hft {

using OrderId = uint32_t;
using TraderId = uint32_t;
using Quantity = uint32_t;
using Price = uint32_t;

constexpr size_t TICKER_SIZE = 4;
using Ticker = std::array<char, TICKER_SIZE>;
using TickerRef = const Ticker &;

struct TickerHash {
  std::size_t operator()(const Ticker &t) const {
    return std::hash<std::string_view>{}(std::string_view(t.data(), t.size()));
  }
};

enum class OrderAction : uint8_t { Buy = 0U, Sell = 1U };
enum class OrderState : uint8_t {
  Accepted = 0U,
  Partial = 1U << 0,
  Full = 1U << 1,
  Instant = 1U << 2
};

struct Order {
  TraderId traderId; // Server side
  OrderId id;
  Ticker ticker{};
  Quantity quantity;
  Price price;
  OrderAction action;
};
using PtrOrder = PoolPtr<Order>;

struct OrderStatus {
  TraderId traderId; // Server side
  OrderId id;
  Ticker ticker{};
  Quantity quantity;
  Price fillPrice;
  OrderState state;
  OrderAction action;
};
using PtrOrderStatus = PoolPtr<OrderStatus>;

struct TickerPrice {
  Ticker ticker{};
  Price price;
};
using PtrTickerPrice = PoolPtr<TickerPrice>;

} // namespace hft

#endif // HFT_COMMON_MARKET_TYPES_HPP
