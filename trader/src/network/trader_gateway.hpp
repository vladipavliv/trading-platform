/**
 * @author Vladimir Pavliv
 * @date 2025-02-13
 */

#ifndef HFT_TRADER_TRADERGATEWAY_HPP
#define HFT_TRADER_TRADERGATEWAY_HPP

#include "bus/bus.hpp"
#include "bus/subscription_holder.hpp"
#include "config/config.hpp"
#include "network/socket_status.hpp"
#include "trader_events.hpp"
#include "types.hpp"

namespace hft::trader {

class TraderGateway {

public:
  TraderGateway(Bus &bus) : bus_{bus}, subs_{id_, bus_.systemBus} {}

  template <typename MessageType>
    requires Bus::MarketBus::RoutedType<MessageType>
  void post(Span<MessageType> messages) {
    if (!authenticated()) {
      Logger::monitorLogger->error("401");
      return;
    }
    bus_.post(messages);
  }

  template <typename MessageType>
    requires(!Bus::MarketBus::RoutedType<MessageType>)
  void post(CRef<MessageType> message) {
    if (!authenticated()) {
      Logger::monitorLogger->error("401");
      return;
    }
    bus_.post(message);
  }

  inline ObjectId id() const { return id_; }

  inline bool authenticated() const { return status_ == SessionStatus::Authenticated; }

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
  SessionStatus status_{SessionStatus::Disconnected};
  String authToken_;

  SubscriptionHolder subs_;
};

template <>
void TraderGateway::post<SocketStatusEvent>(CRef<SocketStatusEvent> event) {
  spdlog::debug([&event] { return utils::toString(event); }());
  if (status_ == SessionStatus::Error) {
    Logger::monitorLogger->error("401");
    return;
  }
  switch (event.status) {
  case SocketStatus::Connected: {
    if (!connectionId_.has_value()) {
      connectionId_ = event.connectionId;
    }
    if (status_ == SessionStatus::Disconnected) {
      status_ = SessionStatus::Connected;
      authenticate();
    }
    break;
  }
  case SocketStatus::Error:
    status_ = SessionStatus::Error;
    break;
  case SocketStatus::Disconnected:
  default:
    status_ = SessionStatus::Disconnected;
    break;
  }
  if (authenticated()) {
    bus_.post<SessionStatusEvent>({event.connectionId, status_});
  }
}

template <>
void TraderGateway::post<LoginResponse>(CRef<LoginResponse> event) {
  spdlog::debug([&event] { return utils::toString(event); }());
  if (status_ != SessionStatus::Connected) {
    Logger::monitorLogger->error("Invalid status for authentication {}", utils::toString(status_));
  } else if (!event.success) {
    Logger::monitorLogger->error("Authentication failed");
    status_ = SessionStatus::Error;
  } else {
    Logger::monitorLogger->info("Logged in, token: {}", event.token);
    status_ = SessionStatus::Authenticated;
    authToken_ = event.token;
  }
}

} // namespace hft::trader

#endif // HFT_TRADER_TRADERGATEWAY_HPP