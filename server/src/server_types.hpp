/**
 * @author Vladimir Pavliv
 * @date 2025-04-18
 */

#ifndef HFT_SERVER_SERVERTYPES_HPP
#define HFT_SERVER_SERVERTYPES_HPP

#include "bus/bus_holder.hpp"
#include "domain_types.hpp"
#include "metadata_types.hpp"
#include "types.hpp"
#include "utils/string_utils.hpp"

namespace hft::server {

/**
 * @brief These server side wrappers are convenient and help keeping domain
 * types clean, but they do introduce additional copy converting domain message
 * to a server-side message. To avoid this - separate serializers could be made
 * so on the server side message gets deserialized directly to a server local type
 * and all the server side fields would get filled afterwards
 */
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

using ServerBus = BusHolder<MessageBus<ServerOrder, ServerOrderStatus, TickerPrice>,
                            DataBus<OrderTimestamp, RuntimeMetrics>>;
} // namespace hft::server

namespace hft::utils {
template <>
inline String toString<server::ServerLoginRequest>(const server::ServerLoginRequest &msg) {
  return std::format("{} {}", msg.connectionId, utils::toString(msg.request));
}
template <>
inline String toString<server::ServerTokenBindRequest>(const server::ServerTokenBindRequest &msg) {
  return std::format("{} {}", msg.connectionId, utils::toString(msg.request));
}
template <>
inline String toString<server::ServerLoginResponse>(const server::ServerLoginResponse &msg) {
  return std::format("{} {} {} {}", msg.connectionId, msg.clientId, msg.ok, msg.error);
}
template <>
inline String toString<server::ServerOrder>(const server::ServerOrder &msg) {
  return std::format("{} {}", msg.clientId, utils::toString(msg.order));
}
template <>
inline String toString<server::ServerOrderStatus>(const server::ServerOrderStatus &msg) {
  return std::format("{} {}", msg.clientId, utils::toString(msg.orderStatus));
}
} // namespace hft::utils

#endif // HFT_SERVER_SERVERTYPES_HPP
