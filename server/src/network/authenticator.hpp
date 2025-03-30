/**
 * @author Vladimir Pavliv
 * @date 2025-03-29
 */

#ifndef HFT_SERVER_AUTHENTICATOR_HPP
#define HFT_SERVER_AUTHENTICATOR_HPP

#include "bus/bus.hpp"
#include "bus/subscription_holder.hpp"
#include "db/postgres_adapter.hpp"
#include "server_events.hpp"
#include "template_types.hpp"
#include "types.hpp"
#include "utils/utils.hpp"

namespace hft::server {

class Authenticator {
public:
  explicit Authenticator(SystemBus &bus) : bus_{bus}, subs_{id_, bus_} {
    bus_.subscribe<LoginRequestEvent>(
        id_, subs_.add<CRefHandler<LoginRequestEvent>>(
                 [this](CRef<LoginRequestEvent> event) { authenticate(event); }));
  }

private:
  void authenticate(CRef<LoginRequestEvent> event) {
    spdlog::debug("Authenticate {} {}", event.request.name, event.request.password);
    LoginResponseEvent response;
    response.connectionId = event.connectionId;

    auto traderId = db::PostgresAdapter::authenticate(event.request);
    if (traderId.has_value()) {
      spdlog::debug("Authentication successfull id: {}", traderId.value());
      response.response.success = true;
      response.response.token = utils::generateToken();
      response.traderId = traderId.value();
    } else {
      spdlog::error("Authentication failed");
    }
    bus_.post<LoginResponseEvent>(response);
  }

private:
  const ObjectId id_{utils::getId()};

  SystemBus &bus_;
  SubscriptionHolder subs_;
};

} // namespace hft::server

#endif // HFT_SERVER_AUTHENTICATOR_HPP