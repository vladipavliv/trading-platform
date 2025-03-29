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

enum class OrderAction : uint8_t { Buy, Sell };
enum class OrderState : uint8_t { Accepted, Partial, Full };

struct Order {
  TraderId traderId; // Server side
  OrderId id;
  Ticker ticker{};
  Quantity quantity;
  Price price;
  OrderAction action;
};

struct OrderStatus {
  TraderId traderId; // Server side
  OrderId id;
  Ticker ticker{};
  Quantity quantity;
  Price fillPrice;
  OrderState state;
  OrderAction action;
};

struct TickerPrice {
  Ticker ticker{};
  Price price;
};

} // namespace hft

#endif // HFT_COMMON_MARKET_TYPES_HPP
