/**
 * @author Vladimir Pavliv
 * @date 2025-04-18
 */

#ifndef HFT_SERVER_SESSIONCHANNEL_HPP
#define HFT_SERVER_SESSIONCHANNEL_HPP

#include "boost_types.hpp"
#include "logging.hpp"
#include "network/async_transport.hpp"
#include "network/channel.hpp"
#include "server_types.hpp"
#include "session_bus.hpp"
#include "types.hpp"

namespace hft::server {

/**
 * @brief Gateway channel for the session
 * @details Uses SessionBus to prevent unauthenticated messages to go through
 */
template <AsyncTransport T>
class SessionChannel {
public:
  using Transport = T;
  using ChannelType = Channel<Transport, SessionBus>;

  SessionChannel(Transport &&transport, ConnectionId id, ServerBus &bus)
      : channel_{std::move(transport), id, sessionBus_}, id_{id}, bus_{bus}, sessionBus_{id, bus} {
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

  ChannelType channel_;
};

} // namespace hft::server

#endif // HFT_SERVER_SESSIONCHANNEL_HPP
