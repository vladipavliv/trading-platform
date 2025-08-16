/**
 * @author Vladimir Pavliv
 * @date 2025-04-18
 */

#ifndef HFT_SERVER_SESSIONCHANNEL_HPP
#define HFT_SERVER_SESSIONCHANNEL_HPP

#include "boost_types.hpp"
#include "logging.hpp"
#include "network/channels/tcp_channel.hpp"
#include "server_types.hpp"
#include "session_bus.hpp"
#include "types.hpp"

namespace hft::server {

/**
 * @brief Gateway channel for the session
 * @details Uses SessionBus to prevent unauthenticated messages to go through
 */
class SessionChannel {
public:
  SessionChannel(ConnectionId id, TcpSocket socket, ServerBus &bus)
      : id_{id}, bus_{bus}, sessionBus_{id, bus}, channel_{id, std::move(socket), sessionBus_} {
    channel_.read();
  }

  inline void authenticate(ClientId clientId) {
    LOG_INFO_SYSTEM("Authenticate channel {} {}", id_, clientId);
    if (isAuthenticated()) {
      LOG_ERROR_SYSTEM("{} is already authenticated", id_);
      return;
    }
    sessionBus_.authenticate(clientId);
  }

  inline auto isAuthenticated() const -> bool { return sessionBus_.isAuthenticated(); }

  inline auto connectionId() const -> ConnectionId { return id_; }

  inline auto clientId() const -> Optional<ClientId> { return sessionBus_.clientId(); }

  inline void close() { channel_.close(); }

  inline void write(CRef<LoginResponse> message) { channel_.write(message); }

  inline void write(CRef<OrderStatus> message) {
    if (!isAuthenticated()) [[unlikely]] {
      LOG_ERROR_SYSTEM("Channel {} is not authenticated", id_);
      return;
    }
    channel_.write(message);
  }

private:
  const ConnectionId id_;

  ServerBus &bus_;
  SessionBus sessionBus_;

  TcpChannel<SessionBus> channel_;
};

} // namespace hft::server

#endif // HFT_SERVER_SESSIONCHANNEL_HPP
