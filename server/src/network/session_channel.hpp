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
#include "network_traits.hpp"
#include "session_bus.hpp"
#include "traits.hpp"
#include "types.hpp"

namespace hft::server {

/**
 * @brief Gateway channel for the session
 * @details Uses SessionBus to prevent unauthenticated messages to go through
 */
class SessionChannel {
  using Chan = Channel<StreamTransport, SessionBus>;

public:
  SessionChannel(StreamTransport &&transport, ConnectionId id, ServerBus &bus)
      : id_{id}, sessionBus_{id, bus}, channel_{std::move(transport), id, sessionBus_} {}

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

  inline void read() { channel_.read(); }

private:
  const ConnectionId id_;

  SessionBus sessionBus_;
  Chan channel_;
};

} // namespace hft::server

#endif // HFT_SERVER_SESSIONCHANNEL_HPP
