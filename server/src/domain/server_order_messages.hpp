/**
 * @author Vladimir Pavliv
 * @date 2025-04-18
 */

#ifndef HFT_SERVER_SERVERDOMAINTYPES_HPP
#define HFT_SERVER_SERVERDOMAINTYPES_HPP

#include "domain_types.hpp"
#include "primitive_types.hpp"

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

#endif // HFT_SERVER_SERVERDOMAINTYPES_HPP
