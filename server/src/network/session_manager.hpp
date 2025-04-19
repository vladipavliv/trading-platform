/**
 * @author Vladimir Pavliv
 * @date 2025-03-29
 */

#ifndef HFT_SERVER_SESSIONMANAGER_HPP
#define HFT_SERVER_SESSIONMANAGER_HPP

#include <folly/AtomicHashMap.h>
#include <folly/container/F14Map.h>

#include "boost_types.hpp"
#include "comparators.hpp"
#include "constants.hpp"
#include "logging.hpp"
#include "network/connection_status.hpp"
#include "network/size_framer.hpp"
#include "network/transport/tcp_transport.hpp"
#include "server_events.hpp"
#include "server_types.hpp"
#include "session_channel.hpp"
#include "utils/utils.hpp"

namespace hft::server {

/**
 * @brief
 */
class SessionManager {
  struct Session {
    ClientId clientId;
    Token token;
    UPtr<SessionChannel> upstreamChannel;
    UPtr<SessionChannel> downstreamChannel;
  };

public:
  explicit SessionManager(Bus &bus)
      : bus_{bus}, unauthorizedUpstreamMap_{MAX_CONNECTIONS},
        unauthorizedDownstreamMap_{MAX_CONNECTIONS}, sessionsMap_{MAX_CONNECTIONS} {
    bus_.marketBus.setHandler<ServerOrderStatus>(
        [this](CRef<ServerOrderStatus> event) { onOrderStatus(event); });
    bus_.systemBus.subscribe<ServerLoginResponse>(
        [this](CRef<ServerLoginResponse> event) { onLoginResponse(event); });
    bus_.systemBus.subscribe<ServerTokenBindRequest>(
        [this](CRef<ServerTokenBindRequest> event) { onTokenBindRequest(event); });
    bus_.systemBus.subscribe<ChannelStatusEvent>(
        [this](CRef<ChannelStatusEvent> event) { onChannelStatus(event); });
  }

  void acceptUpstream(TcpSocket socket) {
    if (sessionsMap_.size() >= MAX_CONNECTIONS) {
      LOG_ERROR("Connection limit reached");
      return;
    }
    const auto id = utils::generateConnectionId();
    auto newTransport = std::make_unique<SessionChannel>(id, std::move(socket), bus_);
    const auto result =
        unauthorizedUpstreamMap_.insert(std::make_pair(id, std::move(newTransport)));
    if (!result.second) {
      LOG_ERROR("Failed to insert new upstream connection");
    } else {
      LOG_INFO_SYSTEM("New upstream connection id:{}", id);
    }
  }

  void acceptDownstream(TcpSocket socket) {
    if (sessionsMap_.size() >= MAX_CONNECTIONS) {
      LOG_ERROR("Connection limit reached");
      return;
    }
    const auto id = utils::generateConnectionId();
    auto newTransport = std::make_unique<SessionChannel>(id, std::move(socket), bus_);
    const auto result =
        unauthorizedDownstreamMap_.insert(std::make_pair(id, std::move(newTransport)));
    if (!result.second) {
      LOG_ERROR("Failed to insert new downstream connection");
    } else {
      LOG_INFO_SYSTEM("New downstream connection Id:{}", id);
    }
  }

private:
  void onOrderStatus(CRef<ServerOrderStatus> status) {
    LOG_DEBUG(utils::toString(status));
    const auto sessionIter = sessionsMap_.find(status.clientId);
    if (sessionIter == sessionsMap_.end() || sessionIter->second.downstreamChannel == nullptr) {
      LOG_INFO("Client {} is offline", status.clientId);
    } else {
      sessionIter->second.downstreamChannel->write(status.orderStatus);
    }
  }

  void onLoginResponse(CRef<ServerLoginResponse> loginResult) {
    LOG_DEBUG("onLoginResponse {} {}", loginResult.ok, loginResult.token);
    const auto channelIter = unauthorizedUpstreamMap_.find(loginResult.connectionId);
    if (channelIter == unauthorizedUpstreamMap_.end()) {
      LOG_ERROR("Connection not found {}", loginResult.connectionId);
      return;
    }
    if (channelIter->second == nullptr) {
      LOG_ERROR("Connection not initialized {}", loginResult.connectionId);
      unauthorizedUpstreamMap_.erase(channelIter->first);
      return;
    }
    if (!loginResult.ok) {
      LOG_ERROR("Authentication failed for {}, closing channel", loginResult.connectionId);
      channelIter->second->write(LoginResponse{0, false, loginResult.error});
      unauthorizedUpstreamMap_.erase(channelIter->first);
    } else {
      const auto token = utils::generateToken();

      Session newSession;
      newSession.clientId = loginResult.clientId;
      newSession.token = token;
      newSession.upstreamChannel = std::move(channelIter->second);
      unauthorizedUpstreamMap_.erase(channelIter->first);

      auto &channelRef = *newSession.upstreamChannel;

      const auto result =
          sessionsMap_.insert(std::make_pair(loginResult.clientId, std::move(newSession)));

      if (!result.second) {
        LOG_ERROR("{} already authorized", loginResult.clientId);
        channelRef.write(LoginResponse{0, false, "Already authorized"});
      } else {
        channelRef.write(LoginResponse{token, true});
        channelRef.authenticate(loginResult.clientId);
      }
    }
  }

  void onTokenBindRequest(CRef<ServerTokenBindRequest> request) {
    LOG_DEBUG("Token bind request {} {}", request.connectionId, request.request.token);
    const auto channelIter = unauthorizedDownstreamMap_.find(request.connectionId);
    if (channelIter == unauthorizedDownstreamMap_.end()) {
      LOG_WARN("Client already disconnected");
      return;
    }
    // Token lookup is only needed at the initial authorization stage
    const auto sessionIter = std::find_if(sessionsMap_.begin(), sessionsMap_.end(),
                                          [token = request.request.token](const auto &session) {
                                            return session.second.token == token;
                                          });
    if (sessionIter == sessionsMap_.end()) {
      LOG_ERROR("Invalid token received from {}", request.connectionId);
      channelIter->second->write(LoginResponse{0, false, "Invalid token"});
    } else if (sessionIter->second.downstreamChannel != nullptr) {
      LOG_ERROR("Downstream channel {} is already connected", request.connectionId);
      channelIter->second->write(LoginResponse{0, false, "Already connected"});
    } else {
      auto &session = sessionIter->second;
      session.downstreamChannel = std::move(channelIter->second);
      session.downstreamChannel->authenticate(session.clientId);
      session.downstreamChannel->write(LoginResponse{request.request.token, true});
      unauthorizedDownstreamMap_.erase(channelIter->first);
      LOG_INFO_SYSTEM("New Session {} {}", sessionIter->first, request.request.token);
      printStats();
    }
  }

  void onChannelStatus(CRef<ChannelStatusEvent> event) {
    LOG_TRACE_SYSTEM(utils::toString(event));
    if (event.event.status == ConnectionStatus::Connected) {
      return;
    }
    unauthorizedUpstreamMap_.erase(event.event.connectionId);
    unauthorizedDownstreamMap_.erase(event.event.connectionId);
    if (event.clientId.has_value() && sessionsMap_.count(event.clientId.value()) > 0) {
      sessionsMap_.erase(event.clientId.value());
      LOG_INFO_SYSTEM("{} disconnected", event.clientId.value());
      printStats();
    }
  }

  inline void printStats() const { LOG_INFO_SYSTEM("Active sessions: {}", sessionsMap_.size()); }

private:
  Bus &bus_;

  folly::AtomicHashMap<ConnectionId, UPtr<SessionChannel>> unauthorizedUpstreamMap_;
  folly::AtomicHashMap<ConnectionId, UPtr<SessionChannel>> unauthorizedDownstreamMap_;

  folly::AtomicHashMap<ClientId, Session> sessionsMap_;
};

} // namespace hft::server

#endif // HFT_SERVER_SESSIONMANAGER_HPP