/**
 * @author Vladimir Pavliv
 * @date 2025-04-18
 */

#ifndef HFT_SERVER_SERVERTYPES_HPP
#define HFT_SERVER_SERVERTYPES_HPP

#include "bus/bus.hpp"
#include "domain_types.hpp"
#include "types.hpp"
#include "utils/string_utils.hpp"

namespace hft::server {

struct ServerLoginRequest {
  ConnectionId connectionId;
  LoginRequest request;
};

struct ServerTokenBindRequest {
  ConnectionId connectionId;
  TokenBindRequest request;
};

struct ServerLoginResponse {
  ConnectionId connectionId;
  ClientId clientId{0};
  bool ok{false};
  String error{""};
};

struct alignas(8) ServerOrder {
  ClientId clientId;
  Order order;
};

struct ServerOrderStatus {
  ClientId clientId;
  OrderStatus orderStatus;
};

using Bus = BusHolder<ServerOrder, ServerOrderStatus, TickerPrice>;
} // namespace hft::server

namespace hft::utils {
template <>
String toString<server::ServerLoginRequest>(const server::ServerLoginRequest &msg) {
  return std::format("{} {}", msg.connectionId, utils::toString(msg.request));
}
template <>
String toString<server::ServerTokenBindRequest>(const server::ServerTokenBindRequest &msg) {
  return std::format("{} {}", msg.connectionId, utils::toString(msg.request));
}
template <>
String toString<server::ServerLoginResponse>(const server::ServerLoginResponse &msg) {
  return std::format("{} {} {} {}", msg.connectionId, msg.clientId, msg.ok, msg.error);
}
template <>
String toString<server::ServerOrder>(const server::ServerOrder &msg) {
  return std::format("{} {}", msg.clientId, utils::toString(msg.order));
}
template <>
String toString<server::ServerOrderStatus>(const server::ServerOrderStatus &msg) {
  return std::format("{} {}", msg.clientId, utils::toString(msg.orderStatus));
}
} // namespace hft::utils

#endif // HFT_SERVER_SERVERTYPES_HPP
