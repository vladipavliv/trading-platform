/**
 * @author Vladimir Pavliv
 * @date 2025-04-18
 */

#ifndef HFT_SERVER_DOMAINSERVERAUTHMESSAGES_HPP
#define HFT_SERVER_DOMAINSERVERAUTHMESSAGES_HPP

#include "domain_types.hpp"
#include "primitive_types.hpp"
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
} // namespace hft::server

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
} // namespace hft

#endif // HFT_SERVER_DOMAINSERVERAUTHMESSAGES_HPP
