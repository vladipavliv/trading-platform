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
 * @brief Channel that acts as a gateway for messages
 * First waits for authentication, then starts routing messages
 * Converts server domain types to/from routed types
 */
class SessionChannel {
public:
  using Transport = TcpTransport<SessionChannel>;

  SessionChannel(ConnectionId id, TcpSocket socket, Bus &bus)
      : id_{id}, transport_{id, std::move(socket), *this}, bus_{bus} {
    transport_.read();
  }

  inline void authenticate(ClientId clientId) {
    if (clientId_.has_value()) {
      LOG_ERROR("{} is already authenticated", id_);
      return;
    }
    clientId_ = clientId;
  }

  template <typename MessageType>
  inline void post(CRef<MessageType> message) {
    LOG_ERROR("Invalid message type received at {}", id_);
  }

  template <typename Type>
  void write(CRef<Type> message) {
    transport_.write(message);
  }

  inline ConnectionId connectionId() const { return id_; }

  inline Opt<ClientId> clientId() const { return clientId_; }

private:
  const ConnectionId id_;

  Bus &bus_;

  Transport transport_;
  Opt<ClientId> clientId_;
};

template <>
inline void SessionChannel::post<LoginRequest>(CRef<LoginRequest> message) {
  if (clientId_.has_value()) {
    LOG_ERROR("Channel {} is already authenticated", id_);
    return;
  }
  bus_.post(ServerLoginRequest{id_, message});
}

template <>
inline void SessionChannel::post<TokenBindRequest>(CRef<TokenBindRequest> message) {
  if (clientId_.has_value()) {
    LOG_ERROR("Channel {} is already authenticated", id_);
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
    LOG_ERROR("Channel {} is not authenticated", id_);
    return;
  }
  bus_.post(ServerOrder{clientId_.value(), message});
}

} // namespace hft::server

#endif // HFT_SERVER_SESSIONCHANNEL_HPP