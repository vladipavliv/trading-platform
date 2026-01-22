/**
 * @author Vladimir Pavliv
 * @date 2025-04-18
 */

#ifndef HFT_SERVER_AUTHENTICATOR_HPP
#define HFT_SERVER_AUTHENTICATOR_HPP

#include "bus/bus_hub.hpp"
#include "domain/server_auth_messages.hpp"
#include "domain/server_order_messages.hpp"
#include "logging.hpp"
#include "traits.hpp"

namespace hft::server {

class Authenticator {
public:
  Authenticator(Context &ctx, DbAdapter &dbAdapter) : ctx_{ctx}, dbAdapter_{dbAdapter} {
    ctx_.bus.subscribe<ServerLoginRequest>(
        CRefHandler<ServerLoginRequest>::template bind<Authenticator, &Authenticator::post>(this));
  }

private:
  void post(CRef<ServerLoginRequest> r) {
    LOG_INFO_SYSTEM("Authenticating {} {}", r.request.name, r.request.password);
    ServerLoginResponse response{r.connectionId};
    const auto result = dbAdapter_.checkCredentials(r.request.name, r.request.password);
    if (result) {
      LOG_INFO_SYSTEM("Authentication successfull");
      response.clientId = *result;
      response.ok = true;
    } else {
      response.error = toString(result.error());
      LOG_ERROR_SYSTEM("Authentication failed {} {}", r.request.name, response.error);
    }
    ctx_.bus.post(response);
  }

private:
  Context &ctx_;
  DbAdapter &dbAdapter_;
};
} // namespace hft::server

#endif // HFT_SERVER_AUTHENTICATOR_HPP