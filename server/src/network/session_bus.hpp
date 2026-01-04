/**
 * @author Vladimir Pavliv
 * @date 2025-08-16
 */

#ifndef HFT_SERVER_SESSIONBUS_HPP
#define HFT_SERVER_SESSIONBUS_HPP

#include "boost_types.hpp"
#include "logging.hpp"
#include "server_events.hpp"
#include "server_types.hpp"
#include "types.hpp"

namespace hft::server {

/**
 * @brief Gateway bus for the session
 * @details First waits for auth message and blocks other types, after successfull
 * auth routes other messages converted to a server-side wrappers
 */
class SessionBus {
public:
  SessionBus(ConnectionId id, ServerBus &bus) : connId_{id}, bus_{bus} {}

  template <typename MessageType>
  inline void post(CRef<MessageType> message) {
    // Supports only the explicitly specialized types
    LOG_ERROR_SYSTEM("Invalid message type received at {}", connId_);
  }

  inline void authenticate(ClientId clientId) { clientId_ = clientId; }

  inline auto isAuthenticated() const -> bool { return clientId_.has_value(); }

  inline auto clientId() const -> Optional<ClientId> { return clientId_; }

private:
  ConnectionId connId_;
  Optional<ClientId> clientId_;

  ServerBus &bus_;
};

template <>
inline void SessionBus::post<LoginRequest>(CRef<LoginRequest> message) {
  if (isAuthenticated()) {
    LOG_ERROR_SYSTEM("Invalid login request: SessionBus for {} is already authenticated", connId_);
    bus_.post(ChannelStatusEvent{clientId_, {connId_, ConnectionStatus::Error}});
    return;
  }
  bus_.post(ServerLoginRequest{connId_, message});
}

template <>
inline void SessionBus::post<TokenBindRequest>(CRef<TokenBindRequest> message) {
  if (isAuthenticated()) {
    LOG_ERROR_SYSTEM("Invalid token bind request: channel {} is already authenticated", connId_);
    bus_.post(ChannelStatusEvent{clientId_, {connId_, ConnectionStatus::Error}});
    return;
  }
  bus_.post(ServerTokenBindRequest{connId_, message});
}

template <>
inline void SessionBus::post<ConnectionStatusEvent>(CRef<ConnectionStatusEvent> event) {
  bus_.post(ChannelStatusEvent{clientId_, event});
}

template <>
inline void SessionBus::post<Order>(CRef<Order> message) {
  if (!isAuthenticated()) [[unlikely]] {
    LOG_ERROR_SYSTEM("Channel {} is not authenticated", connId_);
    bus_.post(ChannelStatusEvent{clientId_, {connId_, ConnectionStatus::Error}});
    return;
  }
  bus_.post(ServerOrder{clientId_.value(), message});
}

} // namespace hft::server

#endif // HFT_SERVER_SESSIONBUS_HPP
