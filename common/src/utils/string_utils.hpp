/**
 * @file utils.hpp
 * @brief Helper functions and stuff
 *
 * @author Vladimir Pavliv
 * @date 2025-02-13
 */

#ifndef HFT_COMMON_STRING_UTILS_HPP
#define HFT_COMMON_STRING_UTILS_HPP

#include <functional>
#include <sstream>
#include <string>

#include "types/market_types.hpp"
#include "types/types.hpp"

namespace hft::utils {

template <typename Type>
std::string toString(const Type &val) {
  return std::to_string(val);
}

template <>
std::string toString<FulfillmentState>(const FulfillmentState &state) {
  switch (state) {
  case FulfillmentState::Full:
    return "Full";
  case FulfillmentState::Partial:
    return "Partial";
  default:
    assert(0);
  }
}

template <>
std::string toString<OrderAction>(const OrderAction &state) {
  switch (state) {
  case OrderAction::Buy:
    return "Buy";
  case OrderAction::Sell:
    return "Sell";
  case OrderAction::Limit:
    return "Limit";
  case OrderAction::Market:
    return "Market";
  default:
    assert(0);
  }
}

template <>
std::string toString<Order>(const Order &order) {
  std::stringstream ss;
  ss << "Order " << toString(order.action) << ": " << order.quantity << " shares of "
     << order.ticker.data() << " at $" << order.price << " each";
  return ss.str();
}

template <>
std::string toString<OrderStatus>(const OrderStatus &order) {
  std::stringstream ss;
  ss << "Order " << order.id << ": "
     << ((order.state == FulfillmentState::Partial) ? "Partially filled " : "Filled ")
     << order.quantity << " shares of " << order.ticker.data() << " at $" << order.fillPrice
     << " each";
  return ss.str();
}

template <>
std::string toString<PriceUpdate>(const PriceUpdate &price) {
  std::stringstream ss;
  ss << price.ticker.data() << ": $" << price.price;
  return ss.str();
}

String toLower(String str) {
  std::transform(str.begin(), str.end(), str.begin(),
                 [](unsigned char c) { return std::tolower(c); });
  return str;
}

} // namespace hft::utils

#endif // HFT_COMMON_STRING_UTILS_HPP