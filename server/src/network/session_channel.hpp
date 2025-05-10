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
 * First waits for authentication, then starts routing converted messages to the bus
 * Has bidirectional bus-like ::post interface and routes messages depending on the type
 */
class SessionChannel {
public:
  using Transport = TcpTransport<SessionChannel>;

  SessionChannel(ConnectionId id, TcpSocket socket, Bus &bus)
      : id_{id}, transport_{id, std::move(socket), *this}, bus_{bus} {
    transport_.read();
  }

  inline void authenticate(ClientId clientId) {
    LOG_INFO_SYSTEM("Authenticate channel {} {}", id_, clientId);
    if (clientId_.has_value()) {
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

  inline auto connectionId() -> ConnectionId const { return id_; }

  inline auto clientId() -> Optional<ClientId> const { return clientId_; }

private:
  const ConnectionId id_;

  Bus &bus_;

  Transport transport_;
  Optional<ClientId> clientId_;
};

template <>
inline void SessionChannel::post<LoginRequest>(CRef<LoginRequest> message) {
  if (clientId_.has_value()) {
    LOG_ERROR_SYSTEM("Invalid login request: channel {} is already authenticated", id_);
    return;
  }
  bus_.post(ServerLoginRequest{id_, message});
}

template <>
inline void SessionChannel::post<TokenBindRequest>(CRef<TokenBindRequest> message) {
  if (clientId_.has_value()) {
    LOG_ERROR_SYSTEM("Invalid token bind request: channel {} is already authenticated", id_);
    return;
  }
  bus_.post(ServerTokenBindRequest{id_, message});
}

template <>
inline void SessionChannel::post<ConnectionStatusEvent>(CRef<ConnectionStatusEvent> event) {
  bus_.post(ChannelStatusEvent{clientId_, event});
}

template <>
inline void SessionChannel::post<Order>(CRef<Order> message) {
  if (!clientId_.has_value()) {
    LOG_ERROR_SYSTEM("Channel {} is not authenticated", id_);
    return;
  }
  bus_.post(ServerOrder{clientId_.value(), message});
}

template <>
inline void SessionChannel::post<LoginResponse>(CRef<LoginResponse> message) {
  transport_.write(message);
}

template <>
inline void SessionChannel::post<OrderStatus>(CRef<OrderStatus> message) {
  if (!clientId_.has_value()) {
    LOG_ERROR_SYSTEM("Channel {} is not authenticated", id_);
    return;
  }
  transport_.write(message);
}

} // namespace hft::server

#endif // HFT_SERVER_SESSIONCHANNEL_HPP
