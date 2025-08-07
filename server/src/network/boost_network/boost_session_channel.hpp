/**
 * @author Vladimir Pavliv
 * @date 2025-04-18
 */

#ifndef HFT_SERVER_SESSIONCHANNEL_HPP
#define HFT_SERVER_SESSIONCHANNEL_HPP

#include "boost_types.hpp"
#include "logging.hpp"
#include "network/transport/tcp_transport.hpp"
#include "server_types.hpp"
#include "types.hpp"

namespace hft::server {

/**
 * @brief Channel that acts as a gateway/sink for messages
 * First waits for authentication message, then accepts other supported messages
 * Converts the domain messages to server-side wrappers with the message specific ids
 * Bidirectional interface routes messages in/out depending on the type
 * @todo which i am not sure is a good idea as it hides the direction of the message
 * Done for convenience. Probably better idea is to make a separate consumer
 * that is a wrapper for SessionChannel to avoid exposing consumer interface, and
 * expose only the interface to send the message downstream.
 */
class BoostSessionChannel {
public:
  using Transport = TcpTransport<BoostSessionChannel>;

  BoostSessionChannel(ConnectionId id, TcpSocket socket, Bus &bus)
      : id_{id}, transport_{id, std::move(socket), *this}, bus_{bus} {
    transport_.read();
  }

  inline void authenticate(ClientId clientId) {
    LOG_INFO_SYSTEM("Authenticate channel {} {}", id_, clientId);
    if (isAuthenticated()) {
      LOG_ERROR_SYSTEM("{} is already authenticated", id_);
      return;
    }
    clientId_ = clientId;
  }

  template <typename MessageType>
  inline void post(CRef<MessageType> message) {
    // Supports only the explicitly specialized types
    LOG_ERROR_SYSTEM("Invalid message type received at {}", id_);
  }

  inline auto isAuthenticated() const -> bool { return clientId_.has_value(); }

  inline auto connectionId() const -> ConnectionId { return id_; }

  inline auto clientId() const -> Optional<ClientId> { return clientId_; }

  void close() {
    transport_.close();
    clientId_.reset();
  }

private:
  const ConnectionId id_;

  Bus &bus_;

  Transport transport_;
  Optional<ClientId> clientId_;
};

template <>
inline void BoostSessionChannel::post<LoginRequest>(CRef<LoginRequest> message) {
  if (isAuthenticated()) {
    LOG_ERROR_SYSTEM("Invalid login request: channel {} is already authenticated", id_);
    return;
  }
  bus_.post(ServerLoginRequest{id_, message});
}

template <>
inline void BoostSessionChannel::post<TokenBindRequest>(CRef<TokenBindRequest> message) {
  if (isAuthenticated()) {
    LOG_ERROR_SYSTEM("Invalid token bind request: channel {} is already authenticated", id_);
    return;
  }
  bus_.post(ServerTokenBindRequest{id_, message});
}

template <>
inline void BoostSessionChannel::post<ConnectionStatusEvent>(CRef<ConnectionStatusEvent> event) {
  bus_.post(ChannelStatusEvent{clientId_, event});
}

template <>
inline void BoostSessionChannel::post<Order>(CRef<Order> message) {
  if (!isAuthenticated()) [[unlikely]] {
    LOG_ERROR_SYSTEM("Channel {} is not authenticated", id_);
    return;
  }
  bus_.post(ServerOrder{clientId_.value(), message});
}

template <>
inline void BoostSessionChannel::post<LoginResponse>(CRef<LoginResponse> message) {
  transport_.write(message);
}

template <>
inline void BoostSessionChannel::post<OrderStatus>(CRef<OrderStatus> message) {
  if (!isAuthenticated()) [[unlikely]] {
    LOG_ERROR_SYSTEM("Channel {} is not authenticated", id_);
    return;
  }
  transport_.write(message);
}

} // namespace hft::server

#endif // HFT_SERVER_SESSIONCHANNEL_HPP
