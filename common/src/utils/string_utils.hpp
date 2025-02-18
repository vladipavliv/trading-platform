/**
 * @author Vladimir Pavliv
 * @date 2025-02-13
 */

#ifndef HFT_COMMON_STRING_UTILS_HPP
#define HFT_COMMON_STRING_UTILS_HPP

#include <functional>
#include <spdlog/spdlog.h>
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
std::string toString<OrderState>(const OrderState &state) {
  switch (state) {
  case OrderState::Full:
    return "Full";
  case OrderState::Partial:
    return "Partial";
  default:
    spdlog::error("Unknown OrderState {}", (uint8_t)state);
  }
  return "";
}

template <>
std::string toString<OrderAction>(const OrderAction &state) {
  switch (state) {
  case OrderAction::Buy:
    return "Buy";
  case OrderAction::Sell:
    return "Sell";
  default:
    spdlog::error("Unknown OrderAction {}", (uint8_t)state);
  }
  return "";
}

template <>
std::string toString<Ticker>(const Ticker &ticker) {
  return std::string(ticker.data(), ticker.size());
}

template <>
std::string toString<Order>(const Order &order) {
  std::stringstream ss;
  ss << toString(order.action) << ": " << order.quantity << " shares of " << toString(order.ticker)
     << " at $" << order.price;
  return ss.str();
}

template <>
std::string toString<OrderStatus>(const OrderStatus &order) {
  std::string state;
  if ((uint8_t)order.state & (uint8_t)OrderState::Partial) {
    state += "Partially ";
  }
  if ((uint8_t)order.state & (uint8_t)OrderState::Full) {
    state += "Fully ";
  }
  if (state.empty()) {
    state = "Accepted ";
  } else {
    state += "filled ";
  }
  return std::format("{:<18}{:<5}{:<6}at ${:<12}", state, order.quantity, order.ticker.data(),
                     order.fillPrice);
}

template <>
std::string toString<TickerPrice>(const TickerPrice &price) {
  std::stringstream ss;
  ss << std::string(price.ticker.begin(), price.ticker.end()) << ": $" << price.price;
  return ss.str();
}

template <typename Type>
std::string toString(const std::vector<Type> &vec) {
  std::stringstream ss;
  for (size_t index = 0; auto &value : vec) {
    ss << toString(value);
    if (index++ < vec.size() - 1) {
      ss << ",";
    }
  }
  return ss.str();
}

String toLower(String str) {
  std::transform(str.begin(), str.end(), str.begin(),
                 [](unsigned char c) { return std::tolower(c); });
  return str;
}

Ticker toTicker(StringRef str) {
  Ticker ticker{};
  std::copy(str.begin(), str.begin() + std::min(str.size(), ticker.size()), ticker.begin());
  return ticker;
}

bool empty(const Ticker &ticker) {
  if (std::all_of(ticker.begin(), ticker.end(), [](char c) { return c == '\0'; })) {
    return true;
  }
  return false;
}

String toString(spdlog::level::level_enum logLvl) {
  switch (logLvl) {
  case spdlog::level::level_enum::trace:
    return "trace";
  case spdlog::level::level_enum::debug:
    return "debug";
  case spdlog::level::level_enum::info:
    return "info";
  case spdlog::level::level_enum::warn:
    return "warn";
  case spdlog::level::level_enum::err:
    return "error";
  case spdlog::level::level_enum::critical:
    return "critical";
  default:
    return "debug";
  }
}

} // namespace hft::utils

#endif // HFT_COMMON_STRING_UTILS_HPP