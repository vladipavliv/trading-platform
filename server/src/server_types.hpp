/**
 * @author Vladimir Pavliv
 * @date 2025-04-18
 */

#ifndef HFT_SERVER_SERVERTYPES_HPP
#define HFT_SERVER_SERVERTYPES_HPP

#include "bus/bus.hpp"
#include "domain_types.hpp"
#include "types.hpp"

namespace hft {

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

} // namespace hft

#endif // HFT_SERVER_SERVERTYPES_HPP
