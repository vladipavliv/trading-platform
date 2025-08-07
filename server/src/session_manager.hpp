/**
 * @author Vladimir Pavliv
 * @date 2025-03-29
 */

#ifndef HFT_SERVER_SESSIONMANAGER_HPP
#define HFT_SERVER_SESSIONMANAGER_HPP

#include <folly/AtomicHashMap.h>
#include <folly/container/F14Map.h>

#include "boost_types.hpp"
#include "constants.hpp"
#include "logging.hpp"
#include "network/connection_status.hpp"
#include "server_events.hpp"
#include "server_types.hpp"
#include "utils/string_utils.hpp"
#include "utils/utils.hpp"

namespace hft::server {

/**
 * @brief Manages sessions, generates tokens, authenticates channels
 */
template <typename SessionChannel, typename BroadcastChannel>
class SessionManager {
  /**
   * @brief Client session info
   * @todo Make rate limiting counter
   */
  struct Session {
    ClientId clientId;
    Token token;
    SPtr<SessionChannel> upstreamChannel;
    SPtr<SessionChannel> downstreamChannel;
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

  void acceptUpstream(SPtr<SessionChannel> channel) {
    LOG_INFO_SYSTEM("New upstream connection id:{}", channel->connectionId());
    if (sessionsMap_.size() >= MAX_CONNECTIONS) {
      LOG_ERROR("Connection limit reached");
      return;
    }
    unauthorizedUpstreamMap_.insert(std::make_pair(channel->connectionId(), channel));
  }

  void acceptDownstream(SPtr<SessionChannel> channel) {
    LOG_INFO_SYSTEM("New downstream connection Id:{}", channel->connectionId());
    if (sessionsMap_.size() >= MAX_CONNECTIONS) {
      LOG_ERROR("Connection limit reached");
      return;
    }
    unauthorizedDownstreamMap_.insert(std::make_pair(channel->connectionId(), channel));
  }

  void acceptBroadcast(SPtr<BroadcastChannel> channel) {
    LOG_INFO_SYSTEM("New broadcast connection Id:{}", channel->connectionId());
    broadcastChannel_ = std::move(channel);
  }

  void close() {
    // Important to keep the order of destruction.
    // SessionManager is created before NetworkServer as the latter needs to reference it
    for (auto iter = sessionsMap_.begin(); iter != sessionsMap_.end(); ++iter) {
      iter->second->upstreamChannel->close();
      iter->second->downstreamChannel->close();
    }
    for (auto iter = unauthorizedUpstreamMap_.begin(); iter != unauthorizedUpstreamMap_.end();
         ++iter) {
      iter->second->close();
    }
    for (auto iter = unauthorizedDownstreamMap_.begin(); iter != unauthorizedDownstreamMap_.end();
         ++iter) {
      iter->second->close();
    }
    sessionsMap_.clear();
    unauthorizedUpstreamMap_.clear();
    unauthorizedDownstreamMap_.clear();
    broadcastChannel_.reset();
  }

private:
  void onOrderStatus(CRef<ServerOrderStatus> status) {
    LOG_DEBUG("{}", utils::toString(status));
    const auto sessionIter = sessionsMap_.find(status.clientId);
    if (sessionIter == sessionsMap_.end()) [[unlikely]] {
      LOG_DEBUG("Client {} is offline", status.clientId);
      return;
    }
    const auto session = sessionIter->second;
    if (session->downstreamChannel != nullptr) [[likely]] {
      session->downstreamChannel->post(status.orderStatus);
    } else {
      LOG_INFO("No downstream connection for {}", status.clientId);
    }
  }

  void onLoginResponse(CRef<ServerLoginResponse> loginResult) {
    LOG_DEBUG("onLoginResponse {} {}", loginResult.ok, loginResult.clientId);
    const auto channelIter = unauthorizedUpstreamMap_.find(loginResult.connectionId);
    if (channelIter == unauthorizedUpstreamMap_.end()) [[unlikely]] {
      LOG_ERROR("Connection not found {}", loginResult.connectionId);
      return;
    }
    // Copy right away to dodge iterator invalidation
    const auto channel = channelIter->second;
    unauthorizedUpstreamMap_.erase(channelIter->first);

    if (channel == nullptr) [[unlikely]] {
      LOG_ERROR("Connection not initialized {}", loginResult.connectionId);
      return;
    }
    if (!loginResult.ok) {
      LOG_ERROR("Authentication failed for {}, closing channel", loginResult.connectionId);
      channel->post(LoginResponse{0, false, loginResult.error});
    } else {
      const auto token = utils::generateToken();

      auto newSession = std::make_shared<Session>();
      newSession->clientId = loginResult.clientId;
      newSession->token = token;
      newSession->upstreamChannel = channel;

      if (!sessionsMap_.insert(std::make_pair(loginResult.clientId, newSession)).second) {
        LOG_ERROR("{} already authorized", loginResult.clientId);
        channel->post(LoginResponse{0, false, "Already authorized"});
      } else {
        newSession->upstreamChannel->post(LoginResponse{token, true});
        newSession->upstreamChannel->authenticate(loginResult.clientId);
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
    const auto downstreamChannel = channelIter->second;
    unauthorizedDownstreamMap_.erase(channelIter->first);

    // Token lookup is only needed at the initial authorization stage, so maintaining a separate
    // lookup container for that might not be needed. More moving parts = more things to break
    const auto sessionIter = std::find_if(
        // formatting comment
        sessionsMap_.begin(), sessionsMap_.end(),
        [token = request.request.token](const auto &session) {
          return session.second->token == token;
        });
    if (sessionIter == sessionsMap_.end()) {
      LOG_ERROR("Invalid token received from {}", request.connectionId);
      downstreamChannel->post(LoginResponse{0, false, "Invalid token"});
    } else {
      const auto session = sessionIter->second;
      if (session->downstreamChannel != nullptr) {
        LOG_ERROR("Downstream channel {} is already connected", request.connectionId);
        downstreamChannel->post(LoginResponse{0, false, "Already connected"});
      } else {
        session->downstreamChannel = std::move(downstreamChannel);
        session->downstreamChannel->authenticate(session->clientId);
        session->downstreamChannel->post(LoginResponse{request.request.token, true});
        LOG_INFO_SYSTEM("New Session {} {}", sessionIter->first, request.request.token);
        printStats();
      }
    }
  }

  void onChannelStatus(CRef<ChannelStatusEvent> event) {
    LOG_TRACE_SYSTEM("{}", utils::toString(event));
    switch (event.event.status) {
    case ConnectionStatus::Connected:
      // Nothing to do here for now
      break;
    case ConnectionStatus::Disconnected:
    case ConnectionStatus::Error:
      LOG_DEBUG("Channel {} disconnected", event.event.connectionId);
      // No info about whether it was upstream or downstream event, but all ids are unique
      // so erase both unauthorized containers, and cleanup session if it was started
      unauthorizedUpstreamMap_.erase(event.event.connectionId);
      unauthorizedDownstreamMap_.erase(event.event.connectionId);
      if (event.clientId.has_value() && sessionsMap_.count(event.clientId.value()) > 0) {
        sessionsMap_.erase(event.clientId.value());
        LOG_INFO_SYSTEM("Client {} disconnected", event.clientId.value());
        printStats();
      }
      break;
    default:
      break;
    }
  }

  inline void printStats() const { LOG_INFO_SYSTEM("Active sessions: {}", sessionsMap_.size()); }

private:
  Bus &bus_;

  folly::AtomicHashMap<ConnectionId, SPtr<SessionChannel>> unauthorizedUpstreamMap_;
  folly::AtomicHashMap<ConnectionId, SPtr<SessionChannel>> unauthorizedDownstreamMap_;

  folly::AtomicHashMap<ClientId, SPtr<Session>> sessionsMap_;

  SPtr<BroadcastChannel> broadcastChannel_;
};

} // namespace hft::server

#endif // HFT_SERVER_SESSIONMANAGER_HPP
