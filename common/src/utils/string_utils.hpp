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

#include "market_types.hpp"
#include "network_types.hpp"
#include "template_types.hpp"
#include "types.hpp"

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

std::string_view toStrView(const Ticker &ticker) {
  return std::string_view(ticker.data(), TICKER_SIZE);
}

template <>
std::string toString<Order>(const Order &order) {
  std::stringstream ss;
  ss << toString(order.action) << " " << order.quantity << " shares of " << toStrView(order.ticker)
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
  return std::format("{} {} {} {} at {}", state, order.id, toString(order.action), order.quantity,
                     toStrView(order.ticker), order.fillPrice);
}

template <>
std::string toString<TickerPrice>(const TickerPrice &price) {
  std::stringstream ss;
  ss << toStrView(price.ticker) << ": $" << price.price;
  return ss.str();
}

template <typename Type>
std::string toString(const std::vector<Type> &vec) {
  std::stringstream ss;
  ss << "[";
  for (size_t index = 0; auto &value : vec) {
    ss << toString(value);
    if (index++ < vec.size() - 1) {
      ss << ",";
    }
  }
  ss << "]";
  return ss.str();
}

template <typename Type>
std::string toString(Span<Type> elements) {
  std::string result;
  for (auto &element : elements) {
    result += toString(element) + " ";
  }
  return result;
}

String toLower(String str) {
  std::transform(str.begin(), str.end(), str.begin(),
                 [](unsigned char c) { return std::tolower(c); });
  return str;
}

Ticker toTicker(CRefString str) {
  Ticker ticker{};
  std::memcpy(ticker.data(), str.data(), std::min(str.size(), TICKER_SIZE));
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

template <typename Value>
LogLevel fromString(const String &input);

template <>
LogLevel fromString<LogLevel>(const String &input) {
  if (input == "trace") {
    return LogLevel::trace;
  }
  if (input == "debug") {
    return LogLevel::debug;
  }
  if (input == "info") {
    return LogLevel::info;
  }
  if (input == "warn") {
    return LogLevel::warn;
  }
  if (input == "err") {
    return LogLevel::err;
  }
  if (input == "critical") {
    return LogLevel::critical;
  }
  if (input == "trace") {
    return LogLevel::trace;
  }
  return LogLevel::trace;
}

String toString(SocketType socketType) {
  switch (socketType) {
  case SocketType::Ingress:
    return "Ingress";
  case SocketType::Egress:
    return "Egress";
  case SocketType::Broadcast:
    return "Broadcast";
  default:
    return "Broadcast";
  }
}

std::string toStringDebug(const OrderStatus &status) {
  return std::format("OrderStatus {} {} {} {} {}", status.id, toStrView(status.ticker),
                     (uint8_t)status.state, status.quantity, status.fillPrice);
}

std::string toStringDebug(const Order &order) {
  return std::format("Order {} {} {} {}", order.id, toStrView(order.ticker), (uint8_t)order.action,
                     order.quantity, order.price);
}

std::string toStringDebug(const TickerPrice &price) {
  return std::format("TickerPrice {} {}", price.price, toStrView(price.ticker));
}

} // namespace hft::utils

#endif // HFT_COMMON_STRING_UTILS_HPP