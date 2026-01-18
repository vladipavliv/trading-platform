/**
 * @author Vladimir Pavliv
 * @date 2025-04-18
 */

#ifndef HFT_SERVER_SESSIONCHANNEL_HPP
#define HFT_SERVER_SESSIONCHANNEL_HPP

#include "bus/bus_restrictor.hpp"
#include "logging.hpp"
#include "primitive_types.hpp"
#include "session_bus.hpp"
#include "traits.hpp"
#include "transport/channel.hpp"

namespace hft::server {

/**
 * @brief Gateway channel for the session
 * @details Uses SessionBus to prevent unauthenticated messages to go through
 */
template <typename BusT>
class SessionChannel {
  using Chan = Channel<StreamTransport, SessionBus<BusT>>;

public:
  SessionChannel(StreamTransport &&transport, ConnectionId id, BusT &&bus)
      : channel_{std::make_shared<Chan>(std::move(transport), id,
                                        SessionBus<BusT>(id, std::move(bus)))} {}

  inline void authenticate(ClientId clientId) {
    LOG_INFO_SYSTEM("Authenticate channel {} {}", channel_->id(), clientId);
    if (isAuthenticated()) {
      LOG_ERROR_SYSTEM("{} is already authenticated", channel_->id());
      return;
    }
    channel_->bus().authenticate(clientId);
  }

  inline auto isAuthenticated() const -> bool { return channel_->bus().isAuthenticated(); }

  inline auto connectionId() const -> ConnectionId { return channel_->id(); }

  inline void close() { channel_->close(); }

  inline void write(CRef<LoginResponse> message) { channel_->write(message); }

  inline void write(CRef<OrderStatus> message) {
    if (!isAuthenticated()) [[unlikely]] {
      LOG_ERROR_SYSTEM("Channel {} is not authenticated", channel_->id());
      return;
    }
    channel_->write(message);
  }

  inline void read() {
    LOG_DEBUG("SessionChannel read");
    channel_->read();
  }

private:
  SPtr<Chan> channel_;
};

} // namespace hft::server

#endif // HFT_SERVER_SESSIONCHANNEL_HPP
