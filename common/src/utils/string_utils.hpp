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

inline String toString(const auto &val) { return std::to_string(static_cast<size_t>(val)); }

inline String toString(const StatusCode &code) {
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

inline String toString(const LoginRequest &msg) {
  return std::format("LoginRequest {} {}", msg.name, msg.password);
}

inline String toString(const TokenBindRequest &msg) {
  return std::format("TokenBindRequest {}", msg.token);
}

inline String toString(const LoginResponse &msg) {
  return std::format("LoginResponse {} {} {}", msg.token, msg.ok, msg.error);
}

inline String toString(const OrderState &state) {
  switch (state) {
  case OrderState::Accepted:
    return "Accepted";
  case OrderState::Rejected:
    return "Rejected";
  case OrderState::Partial:
    return "Partial";
  case OrderState::Full:
    return "Full";
  default:
    LOG_ERROR("Unknown OrderState {}", (uint8_t)state);
  }
  return "";
}

inline String toString(const OrderAction &state) {
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

inline String toString(CRef<OrderTimestamp> event) {
  std::stringstream ss;
  ss << "OrderTimestamp: " << event.orderId << " " << event.created << " " << event.accepted << " "
     << event.notified;
  return ss.str();
}

inline String toString(CRef<MetadataSource> event) {
  switch (event) {
  case MetadataSource::Client:
    return "Client";
  case MetadataSource::Server:
    return "Server";
  default:
    return "Unknown";
  }
}

inline String toString(CRef<RuntimeMetrics> event) {
  std::stringstream ss;
  ss << "RuntimeMetrics: " << toString(event.source) << " " << event.timeStamp << " " << event.rps
     << " " << event.avgLatencyUs;
  return ss.str();
}

inline String toString(CRef<LogEntry> event) {
  std::stringstream ss;
  ss << "LogEntry: " << toString(event.source) << " " << event.message << " " << event.level;
  return ss.str();
}

inline String toString(const Ticker &ticker) { return String(ticker.data(), TICKER_SIZE); }

inline String toString(const Order &o) {
  std::stringstream ss;
  ss << "Order: " << o.id << " " << o.created << " " << toString(o.ticker);
  ss << o.quantity << " " << o.price << " " << toString(o.action);
  return ss.str();
}

inline String toString(const OrderStatus &status) {
  std::stringstream ss;
  ss << "OrderStatus: " << status.orderId << " " << status.timeStamp << " " << status.quantity
     << " " << status.fillPrice << " " << toString(status.state);
  return ss.str();
}

inline String toString(const TickerPrice &price) {
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

inline String toLower(String str) {
  std::transform(str.begin(), str.end(), str.begin(),
                 [](unsigned char c) { return std::tolower(c); });
  return str;
}

inline Ticker toTicker(CRef<String> str) {
  Ticker ticker{};
  std::memcpy(ticker.data(), str.data(), std::min(str.size(), TICKER_SIZE));
  return ticker;
}

inline bool empty(const Ticker &ticker) {
  if (std::all_of(ticker.begin(), ticker.end(), [](char c) { return c == '\0'; })) {
    return true;
  }
  return false;
}

inline String toString(spdlog::level::level_enum logLvl) {
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