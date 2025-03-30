/**
 * @author Vladimir Pavliv
 * @date 2025-03-29
 */

#ifndef HFT_SERVER_SERVERGATEWAY_HPP
#define HFT_SERVER_SERVERGATEWAY_HPP

#include "bus/bus.hpp"
#include "bus/subscription_holder.hpp"
#include "logger.hpp"
#include "network/socket_status.hpp"
#include "server_events.hpp"
#include "template_types.hpp"
#include "utils/template_utils.hpp"

namespace hft::server {

/**
 * @brief Gateway that accepts messages from the socket
 * @details First waits for authorization request, and only after it succeeds
 * starts accepting other requests. When Traderid gets retrieved - supplies it
 * to the messages that require it.
 */
class ServerGateway {
public:
  ServerGateway(Bus &bus) : bus_{bus}, subs_{id_, bus_.systemBus} {}

  template <typename MessageType>
    requires Bus::MarketBus::RoutedType<MessageType>
  void post(Span<MessageType> messages) {
    spdlog::error("ServerGateway {}", [&messages] { return utils::toString(messages); }());
    if (!authenticated()) {
      spdlog::error("401 unauthenticated");
      return;
    }
    if constexpr (utils::HasTraderId<MessageType>::value) {
      for (auto &message : messages) {
        message.traderId = traderId_.value();
      }
    }
    bus_.post(messages);
  }

  template <typename MessageType>
    requires(!Bus::MarketBus::RoutedType<MessageType>)
  void post(CRef<MessageType> message) {
    spdlog::debug(
        "ServerGateway {} {}", [] { return typeid(MessageType).name(); }(),
        [&message] { return utils::toString(message); }());
    if (!authenticated()) {
      spdlog::error("401 unauthenticated");
      return;
    }
    bus_.post(message);
  }

  inline ObjectId id() const { return id_; }

  inline bool authenticated() const {
    return traderId_.has_value() && status_ == SessionStatus::Authenticated;
  }

private:
  void onLoginResponse(CRef<LoginResponseEvent> event) {
    if (authenticated()) {
      Logger::monitorLogger->error("Gateway is already authenticated for {}", traderId_.value());
      return;
    }
    if (event.response.success) {
      Logger::monitorLogger->info("Login succeeded for {}", traderId_.value());
      traderId_ = event.traderId;
    } else {
      Logger::monitorLogger->info("Login failed");
    }
  }

private:
  const ObjectId id_{utils::getId()};

  Bus &bus_;
  SessionStatus status_{SessionStatus::Disconnected};

  std::optional<ObjectId> connectionId_;
  std::optional<TraderId> traderId_;

  SubscriptionHolder subs_;
};

template <>
void ServerGateway::post<SocketStatusEvent>(CRef<SocketStatusEvent> event) {
  spdlog::debug("{}", [&event] { return utils::toString(event); }());
  if (status_ == SessionStatus::Error) {
    spdlog::error("Gateway is in Error state");
    return;
  }
  switch (event.status) {
  case SocketStatus::Connected: {
    if (!connectionId_.has_value()) {
      spdlog::debug("Subscribe to LoginResponseEvent");
      connectionId_ = event.connectionId;
      bus_.systemBus.subscribe<LoginResponseEvent>(
          id_, connectionId_.value(),
          subs_.add<CRefHandler<LoginResponseEvent>>(
              [this](CRef<LoginResponseEvent> event) { onLoginResponse(event); }));
    }
    break;
  }
  case SocketStatus::Disconnected:
  case SocketStatus::Error:
  default:
    // TODO(self) Think it through, maybe this is too much
    status_ = SessionStatus::Disconnected;
    break;
  }
  if (authenticated()) {
    bus_.post<SessionStatusEvent>({traderId_.value(), event.connectionId, status_});
  }
}

template <>
void ServerGateway::post<LoginRequest>(CRef<LoginRequest> request) {
  spdlog::debug("{}", [&request] { return utils::toString(request); }());
  if (authenticated()) {
    Logger::monitorLogger->error("{} already logged in", traderId_.value());
    return;
  }
  bus_.post(LoginRequestEvent{connectionId_.value(), request});
}

template <>
void ServerGateway::post<LoginResponse>(CRef<LoginResponse>) {
  Logger::monitorLogger->error("Invalid LoginResponse message from client");
}

} // namespace hft::server

#endif // HFT_SERVER_SERVERGATEWAY_HPP