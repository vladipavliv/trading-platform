/**
 * @author Vladimir Pavliv
 * @date 2025-02-13
 */

#ifndef HFT_TRADER_TRADERGATEWAY_HPP
#define HFT_TRADER_TRADERGATEWAY_HPP

#include "bus/bus.hpp"
#include "bus/subscription_holder.hpp"
#include "config/config.hpp"
#include "network/connection_status.hpp"
#include "trader_events.hpp"
#include "types.hpp"

namespace hft::trader {

class TraderGateway {
  enum class AuthenticationStatus : uint8_t { None, Requested, Authenticated, Failed };

public:
  TraderGateway(Bus &bus) : bus_{bus}, subs_{id_, bus_.systemBus} {}

  template <typename MessageType>
  void post(Ref<MessageType> message) {
    if (!authenticated()) {
      Logger::monitorLogger->error("401");
      return;
    }
    bus_.post(message);
  }

  template <typename MessageType>
  void post(CRef<MessageType> message) {
    if (!authenticated()) {
      Logger::monitorLogger->error("401");
      return;
    }
    bus_.post(message);
  }

  template <typename MessageType>
  void post(Span<MessageType> messages) {
    if (!authenticated()) {
      Logger::monitorLogger->error("401");
      return;
    }
    bus_.post(messages);
  }

  inline ObjectId id() const { return id_; }

  inline bool authenticated() const { return status_ == AuthenticationStatus::Authenticated; }

private:
  void authenticate() {
    String name = Config::cfg.name;
    String password = Config::cfg.password;
    bus_.post(LoginRequestEvent{connectionId_.value(), {name, password}});
  }

private:
  const ObjectId id_{utils::getId()};

  Bus &bus_;

  std::optional<ObjectId> connectionId_;
  AuthenticationStatus status_{AuthenticationStatus::None};
  String authToken_;

  SubscriptionHolder subs_;
};

template <>
void TraderGateway::post<TcpStatusEvent>(CRef<TcpStatusEvent> event) {
  if (status_ == AuthenticationStatus::Failed) {
    Logger::monitorLogger->error("401");
    return;
  }
  switch (event.status) {
  case ConnectionStatus::Connected: {
    if (!connectionId_.has_value()) {
      connectionId_ = event.id;
    }
    if (status_ == AuthenticationStatus::None) {
      authenticate();
      status_ = AuthenticationStatus::Requested;
    }
    break;
  }
  default:
    // Reset authentication if got disconnected or transport error happened
    status_ = AuthenticationStatus::None;
    break;
  }
  // Forward to the Bus
  bus_.post(event);
}

template <>
void TraderGateway::post<LoginResponse>(CRef<LoginResponse> event) {
  if (status_ != AuthenticationStatus::Requested) {
    Logger::monitorLogger->error("Authentication not requested");
  } else if (!event.success) {
    Logger::monitorLogger->error("Authentication failed");
    status_ = AuthenticationStatus::Failed;
  } else {
    Logger::monitorLogger->error("Authentication success");
    status_ = AuthenticationStatus::Authenticated;
    authToken_ = event.token;
  }
  // Forward to the Bus
  bus_.post(event);
}

} // namespace hft::trader

#endif // HFT_TRADER_TRADERGATEWAY_HPP