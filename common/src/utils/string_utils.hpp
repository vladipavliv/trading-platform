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
std::string toString<Order>(const Order &order) {
  std::stringstream ss;
  ss << "Order id: " << order.id << ", Ticker: " << order.ticker.data()
     << ", Action: " << static_cast<uint8_t>(order.action) << ", Quantity: " << order.quantity
     << ", Price: " << order.price;
  return ss.str();
}

template <>
std::string toString<OrderStatus>(const OrderStatus &order) {
  std::stringstream ss;
  ss << "OrderStatus Id: " << order.id << ", State: " << static_cast<uint8_t>(order.state)
     << ", Quantity: " << order.quantity << ", FillPrice: " << order.fillPrice;
  return ss.str();
}

String toLower(String str) {
  std::transform(str.begin(), str.end(), str.begin(),
                 [](unsigned char c) { return std::tolower(c); });
  return str;
}

} // namespace hft::utils

#endif // HFT_COMMON_STRING_UTILS_HPP