/**
 * @author Vladimir Pavliv
 * @date 2025-08-16
 */

#ifndef HFT_SERVER_SESSIONBUS_HPP
#define HFT_SERVER_SESSIONBUS_HPP

#include "events.hpp"
#include "logging.hpp"
#include "primitive_types.hpp"
#include "traits.hpp"

namespace hft::server {

/**
 * @brief Gateway bus for the session
 * @details First waits for auth message and blocks other types, after successfull
 * auth routes other messages converted to a server-side wrappers
 */
template <typename BusT>
class SessionBus {
public:
  SessionBus(ConnectionId id, BusT &&bus) : connId_{id}, bus_{std::move(bus)} {}

  inline void authenticate(ClientId clientId) { clientId_ = clientId; }

  inline auto isAuthenticated() const -> bool { return clientId_.has_value(); }

  template <typename MessageType>
  inline void post(CRef<MessageType> message) {
    if constexpr (std::is_same_v<MessageType, LoginRequest>) {
      if (isAuthenticated()) {
        LOG_ERROR_SYSTEM("Already authenticated on session {}", connId_);
        bus_.post(ChannelStatusEvent{clientId_, {connId_, ConnectionStatus::Error}});
        return;
      }
      bus_.post(ServerLoginRequest{connId_, message});
    } else if constexpr (std::is_same_v<MessageType, TokenBindRequest>) {
      if (isAuthenticated()) {
        LOG_ERROR_SYSTEM("Already authenticated on session {}", connId_);
        bus_.post(ChannelStatusEvent{clientId_, {connId_, ConnectionStatus::Error}});
        return;
      }
      bus_.post(ServerTokenBindRequest{connId_, message});
    } else if constexpr (std::is_same_v<MessageType, Order>) {
      if (!isAuthenticated()) {
        LOG_ERROR_SYSTEM("Not authenticated: session {}", connId_);
        bus_.post(ChannelStatusEvent{clientId_, {connId_, ConnectionStatus::Error}});
        return;
      }
      bus_.post(ServerOrder{clientId_.value(), message});
    } else if constexpr (std::is_same_v<MessageType, ConnectionStatusEvent>) {
      bus_.post(ChannelStatusEvent{clientId_, message});
    } else {
      LOG_ERROR_SYSTEM("Invalid message type received at {}", connId_);
    }
  }

private:
  ConnectionId connId_;
  Optional<ClientId> clientId_;

  BusT bus_;
};

} // namespace hft::server

#endif // HFT_SERVER_SESSIONBUS_HPP
