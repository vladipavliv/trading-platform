/**
 * @author Vladimir Pavliv
 * @date 2025-04-18
 */

#ifndef HFT_SERVER_AUTHENTICATOR_HPP
#define HFT_SERVER_AUTHENTICATOR_HPP

#include "adapters/adapters.hpp"
#include "logging.hpp"
#include "server_types.hpp"

namespace hft::server {

class Authenticator {
public:
  Authenticator(SystemBus &bus, adapters::DbAdapter &dbAdapter) : bus_{bus}, dbAdapter_{dbAdapter} {
    bus_.subscribe<ServerLoginRequest>(
        [this](CRef<ServerLoginRequest> request) { onAuthenticate(request); });
  }

private:
  void onAuthenticate(CRef<ServerLoginRequest> r) {
    LOG_INFO_SYSTEM("Authenticating {} {}", r.request.name, r.request.password);
    ServerLoginResponse response{r.connectionId};
    const auto result = dbAdapter_.checkCredentials(r.request.name, r.request.password);
    if (result) {
      LOG_INFO_SYSTEM("Authentication successfull");
      response.clientId = *result;
      response.ok = true;
    } else {
      response.error = utils::toString(result.error());
      LOG_ERROR_SYSTEM("Authentication failed {} {}", r.request.name, response.error);
    }
    bus_.post(response);
  }

private:
  SystemBus &bus_;
  adapters::DbAdapter &dbAdapter_;
};
} // namespace hft::server

#endif // HFT_SERVER_AUTHENTICATOR_HPP