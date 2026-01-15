/**
 * @author Vladimir Pavliv
 * @date 2025-04-18
 */

#ifndef HFT_SERVER_SERVERDOMAINTYPES_HPP
#define HFT_SERVER_SERVERDOMAINTYPES_HPP

#include "domain_types.hpp"
#include "primitive_types.hpp"
#include "utils/string_utils.hpp"

namespace hft::server {
struct ServerOrder {
  ClientId clientId;
  Order order;
};

struct ServerOrderStatus {
  ClientId clientId;
  OrderStatus orderStatus;
};
} // namespace hft::server

namespace hft {
inline String toString(const server::ServerOrder &msg) {
  return std::format("{} {}", msg.clientId, toString(msg.order));
}
inline String toString(const server::ServerOrderStatus &msg) {
  return std::format("{} {}", msg.clientId, toString(msg.orderStatus));
}
} // namespace hft

#endif // HFT_SERVER_SERVERDOMAINTYPES_HPP
