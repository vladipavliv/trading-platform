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

#include "logging.hpp"
#include "market_types.hpp"
#include "metadata_types.hpp"
#include "network_types.hpp"
#include "template_types.hpp"
#include "types.hpp"

namespace hft::utils {

String toString(const auto &val) { return std::to_string(val); }

String toString(const CredentialsLoginRequest &msg) {
  return std::format("CredentialsLoginRequest {} {} {}", msg.socketId, msg.name, msg.password);
}

String toString(const TokenLoginRequest &msg) {
  return std::format("TokenLoginRequest {} {}", msg.socketId, msg.token);
}

String toString(const LoginResponse &msg) {
  return std::format("LoginResponse {} {} {} {}", msg.socketId, msg.traderId, msg.token,
                     msg.success);
}

String toString(const OrderState &state) {
  switch (state) {
  case OrderState::Full:
    return "Full";
  case OrderState::Partial:
    return "Partial";
  default:
    LOG_ERROR("Unknown OrderState {}", (uint8_t)state);
  }
  return "";
}

String toString(const OrderAction &state) {
  switch (state) {
  case OrderAction::Buy:
    return "Buy";
  case OrderAction::Sell:
    return "Sell";
  default:
    LOG_ERROR("Unknown OrderAction {}", (uint8_t)state);
  }
  return "";
}

String toString(TimestampType type) {
  switch (type) {
  case TimestampType::Created:
    return "created";
  case TimestampType::Received:
    return "received";
  case TimestampType::Fulfilled:
    return "fulfilled";
  case TimestampType::Notified:
    return "notified";
  default:
    LOG_ERROR("Unknown TimestampType {}", (uint8_t)type);
  }
  return "";
}

String toString(CRef<OrderTimestamp> event) {
  std::stringstream ss;
  ss << "OrderTimestamp " << event.orderId << " " << event.timestamp << " "
     << utils::toString(event.type);
  return ss.str();
}

String toString(const Ticker &ticker) { return String(ticker.data(), TICKER_SIZE); }

String toString(const Order &o) {
  std::stringstream ss;
  ss << o.traderId << " " << o.token << " " << o.id << " " << o.timestamp << " ";
  ss << toString(o.ticker) << o.quantity << " " << o.price << " " << (uint8_t)o.action;
  return ss.str();
}

String toString(const OrderStatus &os) {
  std::stringstream ss;
  ss << os.traderId << " " << os.orderId;
  ss << toString(os.ticker) << os.quantity << " " << os.fillPrice << " ";
  ss << (uint8_t)os.state;
  return ss.str();
}

String toString(const TickerPrice &price) {
  std::stringstream ss;
  ss << toString(price.ticker) << ": $" << price.price;
  return ss.str();
}

template <typename Type>
String toString(const std::vector<Type> &vec) {
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
String toString(Span<Type> elements) {
  String result;
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

Ticker toTicker(CRef<String> str) {
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

} // namespace hft::utils

#endif // HFT_COMMON_STRING_UTILS_HPP