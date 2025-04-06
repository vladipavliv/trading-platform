/**
 * @author Vladimir Pavliv
 * @date 2025-03-29
 */

#ifndef HFT_SERVER_GATEWAY_HPP
#define HFT_SERVER_GATEWAY_HPP

#include <folly/AtomicHashMap.h>
#include <folly/container/F14Map.h>

#include "bus/bus.hpp"
#include "comparators.hpp"
#include "constants.hpp"
#include "logging.hpp"
#include "network/size_framer.hpp"
#include "network/socket_status.hpp"
#include "network/transport/tcp_transport.hpp"
#include "network_types.hpp"
#include "serialization/flat_buffers/fb_serializer.hpp"
#include "server_events.hpp"
#include "template_types.hpp"
#include "utils/template_utils.hpp"

namespace hft::server {

/**
 * @brief
 */
class Gateway {
  using Transport = TcpTransport<Gateway>;

  struct Session {
    TraderId traderId;
    Opt<SocketId> upstreamId;
    Opt<SocketId> downstreamId;
  };

  using SocketMap = folly::AtomicHashMap<SocketId, UPtr<Transport>>;
  using SessionMap = folly::AtomicHashMap<SessionToken, Session>;

public:
  explicit Gateway(Bus &bus)
      : bus_{bus}, upstreamMap_{MAX_CONNECTIONS}, downstreamMap_{MAX_CONNECTIONS},
        sessionsMap_{MAX_CONNECTIONS} {
    bus_.marketBus.setHandler<OrderStatus>(
        [this](CRef<OrderStatus> status) { onOrderStatus(status); });
    bus_.systemBus.subscribe<LoginResponse>(
        [this](CRef<LoginResponse> response) { onLoginResponse(response); });
  }

  void acceptUpstream(TcpSocket socket) {
    if (upstreamMap_.size() >= MAX_CONNECTIONS) {
      LOG_ERROR("Upstream connection limit reached");
      return;
    }
    auto newTransport = std::make_unique<Transport>(std::move(socket), *this);
    const auto id = newTransport->id();
    const auto result = upstreamMap_.insert(std::make_pair(id, std::move(newTransport)));
    if (!result.second) {
      LOG_ERROR("Failed to insert new upstream connection");
    } else {
      LOG_INFO_SYSTEM("New upstream connection id:{}", id);
      result.first->second->read();
    }
  }

  void acceptDownstream(TcpSocket socket) {
    if (downstreamMap_.size() >= MAX_CONNECTIONS) {
      LOG_ERROR("Downstream connection limit reached");
      return;
    }
    auto newTransport = std::make_unique<Transport>(std::move(socket), *this);
    const auto id = newTransport->id();
    const auto result = downstreamMap_.insert(std::make_pair(id, std::move(newTransport)));
    if (!result.second) {
      LOG_ERROR("Failed to insert new downstream connection");
    } else {
      LOG_INFO_SYSTEM("New downstream connection id:{}", id);
      result.first->second->read();
    }
  }

  template <typename MessageType>
  void post(CRef<MessageType> message) {
    LOG_DEBUG("{}", utils::toString(message));

    if constexpr (HasToken<MessageType>) {
      const auto sessionIter = sessionsMap_.find(message.token);
      if (sessionIter == sessionsMap_.end()) {
        LOG_ERROR("Connection is not authenticated");
        return;
      }
      bus_.post(message);
    } else {
      LOG_ERROR("Unsupported message type");
      return;
    }
  }

  void post(CRef<SocketStatusEvent> event) {
    LOG_TRACE_SYSTEM("{}", utils::toString(event));
    if (event.status == SocketStatus::Connected) {
      return;
    }
    // Cleanup disconnected socket from connections map and session map
    for (auto &session : sessionsMap_) {
      Session &s = session.second;
      // Up or Downstream socket
      if (s.upstreamId.has_value() && s.upstreamId == event.socketId) {
        upstreamMap_.erase(event.socketId);
        if (s.downstreamId.has_value()) {
          downstreamMap_.erase(s.downstreamId.value());
        }
        LOG_INFO_SYSTEM("TraderId: {} disconnected", s.traderId);
        sessionsMap_.erase(session.first);
        printStats();
        break;
      }
      if (s.downstreamId.has_value() && s.downstreamId == event.socketId) {
        downstreamMap_.erase(event.socketId);
        if (s.upstreamId.has_value()) {
          upstreamMap_.erase(s.upstreamId.value());
        }
        LOG_INFO_SYSTEM("TraderId: {} disconnected", s.traderId);
        sessionsMap_.erase(session.first);
        printStats();
        break;
      }
    }
  }

  void post(CRef<CredentialsLoginRequest> request) {
    LOG_DEBUG("{}", utils::toString(request));
    bus_.post(request);
  }

  void post(CRef<TokenLoginRequest> request) {
    LOG_DEBUG_SYSTEM("{}", utils::toString(request));
    // Login via token is done on the downstream socket
    const auto sessionIter = sessionsMap_.find(request.token);
    if (sessionIter == sessionsMap_.end()) {
      LOG_ERROR("Invalid token");
      return;
    }
    if (sessionIter->second.downstreamId.has_value()) {
      LOG_ERROR("Connection is already authenticated");
      return;
    }
    // Send response to authenticated socket
    const auto downstreamIt = downstreamMap_.find(request.socketId);
    if (downstreamIt == downstreamMap_.end()) {
      LOG_ERROR("Connection not found");
      return;
    }
    sessionIter->second.downstreamId = request.socketId;
    downstreamIt->second->write(LoginResponse{0, 0, request.token, true});

    LOG_INFO_SYSTEM("Session started Trader: {} Token: {}, UpId: {}, DownId: {}",
                    sessionIter->second.traderId, request.token,
                    sessionIter->second.upstreamId.value_or(0),
                    sessionIter->second.downstreamId.value_or(0));
    printStats();
  }

private:
  void onOrderStatus(CRef<OrderStatus> status) {
    LOG_DEBUG("{}", utils::toString(status));

    auto sessionIt = sessionsMap_.find(status.token);
    if (sessionIt == sessionsMap_.end()) {
      // Could have been an old order and trader logged in with new session
      sessionIt = std::find_if(sessionsMap_.begin(), sessionsMap_.end(),
                               [traderId = status.traderId](const SessionMap::value_type &session) {
                                 return session.second.traderId == traderId;
                               });
    }
    if (sessionIt != sessionsMap_.end() && sessionIt->second.downstreamId.has_value()) {
      auto socketIter = downstreamMap_.find(sessionIt->second.downstreamId.value());
      if (socketIter != downstreamMap_.end()) {
        socketIter->second->write(status);
      } else {
        LOG_ERROR("Downstream socket not found {}", sessionIt->second.downstreamId.value_or(0));
      }
    } else {
      LOG_DEBUG("{} is offline", status.traderId);
    }
  }

  void onLoginResponse(CRef<LoginResponse> response) {
    LOG_DEBUG("onLoginResponse {} {}", response.success, response.token);
    auto socketIter = upstreamMap_.find(response.socketId);
    if (socketIter == upstreamMap_.end()) {
      LOG_ERROR("Socket not found {}", response.socketId);
      return;
    }

    const auto token = utils::generateSessionToken();
    response.setToken(token);

    Session newSession;
    newSession.traderId = response.traderId;
    newSession.upstreamId = socketIter->first;
    auto result = sessionsMap_.insert(std::make_pair(token, std::move(newSession)));
    if (!result.second) {
      LOG_ERROR("Failed to insert new session");
      upstreamMap_.erase(socketIter->first);
      return;
    }
    socketIter->second->write(response);
  }

  void printStats() {
    const auto sc = sessionsMap_.size();
    const auto uc = upstreamMap_.size();
    const auto dc = downstreamMap_.size();
    LOG_INFO_SYSTEM("Sessions: {} , Connections: Up {} Down {}", sc, uc, dc);
  }

private:
  Bus &bus_;

  SocketMap upstreamMap_;
  SocketMap downstreamMap_;
  SessionMap sessionsMap_;
};

} // namespace hft::server

#endif // HFT_SERVER_GATEWAY_HPP