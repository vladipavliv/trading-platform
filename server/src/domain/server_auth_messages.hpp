/**
 * @author Vladimir Pavliv
 * @date 2025-04-18
 */

#ifndef HFT_SERVER_DOMAINSERVERAUTHMESSAGES_HPP
#define HFT_SERVER_DOMAINSERVERAUTHMESSAGES_HPP

#include "domain_types.hpp"
#include "primitive_types.hpp"

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

#endif // HFT_SERVER_DOMAINSERVERAUTHMESSAGES_HPP
