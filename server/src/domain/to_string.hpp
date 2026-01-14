/**
 * @author Vladimir Pavliv
 * @date 2025-04-18
 */

#ifndef HFT_SERVER_DOMAINTOSTRING_HPP
#define HFT_SERVER_DOMAINTOSTRING_HPP

#include "primitive_types.hpp"
#include "server_auth_messages.hpp"
#include "server_order_messages.hpp"
#include "utils/string_utils.hpp"

namespace hft {
inline String toString(const server::ServerLoginRequest &msg) {
  return std::format("{} {}", msg.connectionId, toString(msg.request));
}
inline String toString(const server::ServerTokenBindRequest &msg) {
  return std::format("{} {}", msg.connectionId, toString(msg.request));
}
inline String toString(const server::ServerLoginResponse &msg) {
  return std::format("{} {} {} {}", msg.connectionId, msg.clientId, msg.ok, msg.error);
}
inline String toString(const server::ServerOrder &msg) {
  return std::format("{} {}", msg.clientId, toString(msg.order));
}
inline String toString(const server::ServerOrderStatus &msg) {
  return std::format("{} {}", msg.clientId, toString(msg.orderStatus));
}
} // namespace hft

#endif // HFT_SERVER_DOMAINTOSTRING_HPP
