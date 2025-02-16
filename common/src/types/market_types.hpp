/**
 * @file market_types.hpp
 * @brief Market types
 *
 * @author Vladimir Pavliv
 * @date 2025-02-13
 */

#ifndef HFT_COMMON_MARKET_TYPES_HPP
#define HFT_COMMON_MARKET_TYPES_HPP

#include <algorithm>
#include <cstring>

#include "types.hpp"

namespace hft {

using OrderId = uint64_t;
using Timestamp = uint64_t;
using TraderId = uint32_t;
using Quantity = uint32_t;
using Price = float;
using Ticker = std::array<char, 5>;

enum class OrderAction : uint8_t { Buy = 0U, Sell = 1U };
enum class FulfillmentState : uint8_t { Partial = 0U, Full = 1U };

struct Order {
  TraderId traderId; // Server side
  OrderId id;
  Ticker ticker{};
  OrderAction action{OrderAction::Buy};
  Quantity quantity;
  Price price;
};

struct OrderStatus {
  TraderId traderId; // Server side
  OrderId id;
  Ticker ticker{};
  FulfillmentState state;
  Quantity quantity;
  Price fillPrice;
};

struct PriceUpdate {
  Ticker ticker{};
  Price price;
};

constexpr size_t MAX_MESSAGE_SIZE =
    std::max({sizeof(Order), sizeof(OrderStatus), sizeof(PriceUpdate)}) + sizeof(uint16_t);

} // namespace hft

#endif // HFT_COMMON_MARKET_TYPES_HPP