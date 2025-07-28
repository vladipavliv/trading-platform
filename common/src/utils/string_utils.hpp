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

#include "boost_types.hpp"
#include "domain_types.hpp"
#include "logging.hpp"
#include "metadata_types.hpp"
#include "status_code.hpp"
#include "types.hpp"

namespace hft::utils {

String toString(const auto &val) { return std::to_string(val); }

String toString(const StatusCode &code) {
  switch (code) {
  case StatusCode::Ok:
    return "ok";
  case StatusCode::Error:
    return "error";
  case StatusCode::DbError:
    return "database error";
  case StatusCode::AuthInvalidPassword:
    return "invalid password";
  case StatusCode::AuthUserNotFound:
    return "user not found";
  default:
    LOG_ERROR("Unknown StatusCode {}", (uint8_t)code);
  }
  return "";
}

String toString(const LoginRequest &msg) {
  return std::format("LoginRequest {} {}", msg.name, msg.password);
}

String toString(const TokenBindRequest &msg) {
  return std::format("TokenBindRequest {}", msg.token);
}

String toString(const LoginResponse &msg) {
  return std::format("LoginResponse {} {} {}", msg.token, msg.ok, msg.error);
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

String toString(CRef<OrderTimestamp> event) {
  std::stringstream ss;
  ss << "OrderTimestamp id:" << event.orderId << " created:" << event.created
     << " fulfilled:" << event.fulfilled << " notified:" << event.notified;
  return ss.str();
}

String toString(CRef<MetadataSource> event) {
  switch (event) {
  case MetadataSource::Client:
    return "Client";
  case MetadataSource::Server:
    return "Server";
  default:
    return "Unknown";
  }
}

String toString(CRef<RuntimeMetrics> event) {
  std::stringstream ss;
  ss << "RuntimeMetrics source:" << toString(event.source) << " timestamp:" << event.timeStamp
     << " rps:" << event.rps << " avg latency:" << event.avgLatencyUs;
  return ss.str();
}

String toString(CRef<LogEntry> event) {
  std::stringstream ss;
  ss << "LogEntry source:" << toString(event.source) << " message:" << event.message
     << " level:" << event.level;
  return ss.str();
}

String toString(const Ticker &ticker) { return String(ticker.data(), TICKER_SIZE); }

String toString(const Order &o) {
  std::stringstream ss;
  ss << o.id << " " << o.created << " " << toString(o.ticker);
  ss << o.quantity << " " << o.price << " " << (uint8_t)o.action;
  return ss.str();
}

String toString(const OrderStatus &os) {
  std::stringstream ss;
  ss << os.orderId << os.quantity << " " << os.fillPrice << " ";
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