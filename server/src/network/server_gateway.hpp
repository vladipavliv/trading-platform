/**
 * @author Vladimir Pavliv
 * @date 2025-03-29
 */

#ifndef HFT_SERVER_SERVERGATEWAY_HPP
#define HFT_SERVER_SERVERGATEWAY_HPP

#include "bus/bus.hpp"
#include "bus/subscription_holder.hpp"
#include "logger.hpp"
#include "network/connection_status.hpp"
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
  void post(Ref<MessageType> message) {
    if (!authenticated()) {
      Logger::monitorLogger->error("401");
      return;
    }
    if constexpr (utils::HasTraderId<MessageType>::value) {
      message.traderId = traderId_.value();
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
    if constexpr (utils::HasTraderId<MessageType>::value) {
      for (auto &message : messages) {
        message.traderId = traderId_.value();
      }
    }
    bus_.post(messages);
  }

  inline ObjectId id() const { return id_; }

private:
  inline bool authenticated() const { return traderId_.has_value(); }

  void onLoginResult(CRef<LoginResultEvent> event) {
    if (authenticated()) {
      Logger::monitorLogger->error("Gateway is already authenticated for {}", traderId_.value());
      return;
    }
    if (event.success) {
      traderId_ = event.traderId;
    }
  }

private:
  const ObjectId id_{utils::getId()};

  Bus &bus_;
  SubscriptionHolder subs_;

  std::optional<ObjectId> connectionId_;
  std::optional<TraderId> traderId_;
};

template <>
void ServerGateway::post<TcpStatusEvent>(CRef<TcpStatusEvent> event) {
  if (event.status == ConnectionStatus::Connected && !connectionId_.has_value()) {
    connectionId_ = event.id;
    bus_.systemBus.subscribe<LoginResultEvent>(
        id_, connectionId_.value(),
        subs_.add<CRefHandler<LoginResultEvent>>(
            [this](CRef<LoginResultEvent> event) { onLoginResult(event); }));
  }
  if (authenticated()) {
    TcpStatusEvent newEvent = event;
    newEvent.traderId = traderId_.value();
    bus_.post(newEvent);
  } else {
    bus_.post(event);
  }
}

template <>
void ServerGateway::post<LoginRequest>(CRef<LoginRequest> message) {
  if (authenticated()) {
    Logger::monitorLogger->error("{} already logged in", traderId_.value());
    return;
  }
  bus_.post(LoginRequestEvent{id(), message});
}

template <>
void ServerGateway::post<LoginRequest>(Span<LoginRequest> messages) {
  if (authenticated()) {
    Logger::monitorLogger->error("{} already logged in", traderId_.value());
    return;
  }
  if (messages.size() != 1) {
    Logger::monitorLogger->error("Multiple login attempts");
    return;
  }
  post(messages.front());
}

} // namespace hft::server

#endif // HFT_SERVER_SERVERGATEWAY_HPP