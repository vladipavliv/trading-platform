/**
 * @author Vladimir Pavliv
 * @date 2025-03-29
 */

#ifndef HFT_SERVER_NETWORKSESSIONMANAGER_HPP
#define HFT_SERVER_NETWORKSESSIONMANAGER_HPP

#include <boost/unordered/unordered_flat_map.hpp>
#include <folly/AtomicHashMap.h>
#include <folly/container/F14Map.h>

#include "bus/bus_hub.hpp"
#include "constants.hpp"
#include "containers/vyukov_mpmc.hpp"
#include "events.hpp"
#include "ipc/session_channel.hpp"
#include "logging.hpp"
#include "traits.hpp"
#include "transport/connection_status.hpp"
#include "utils/id_utils.hpp"
#include "utils/string_utils.hpp"

namespace hft::server {

/**
 * @brief Manages sessions, generates tokens, authenticates channels
 */
class NetworkSessionManager {
  using SelfT = NetworkSessionManager;
  using UpstreamChan = SessionChannel<UpstreamBus>;
  using DownstreamChan = SessionChannel<DownstreamBus>;

  static constexpr size_t DRAIN_CHUNK = 1024;
  /**
   * @brief Client session info
   * @todo Make rate limiting counter
   */
  struct Session {
    ClientId clientId;
    Token token;
    SPtr<UpstreamChan> upstreamChannel;
    SPtr<DownstreamChan> downstreamChannel;
  };

public:
  explicit NetworkSessionManager(Context &ctx)
      : ctx_{ctx}, unauthorizedUpstreamMap_{MAX_CONNECTIONS},
        unauthorizedDownstreamMap_{MAX_CONNECTIONS}, sessionsMap_{MAX_CONNECTIONS} {
    LOG_INFO_SYSTEM("NetworkSessionManager initialized");

    ctx_.bus.subscribe(CRefHandler<ServerOrderStatus>::bind<SelfT, &SelfT::post>(this));
    ctx_.bus.subscribe(CRefHandler<ServerLoginResponse>::bind<SelfT, &SelfT::post>(this));
    ctx_.bus.subscribe(CRefHandler<ChannelStatusEvent>::bind<SelfT, &SelfT::post>(this));
    ctx_.bus.subscribe(CRefHandler<ServerTokenBindRequest>::bind<SelfT, &SelfT::post>(this));
  }

  ~NetworkSessionManager() {
    LOG_DEBUG_SYSTEM("~NetworkSessionManager");
    close();
  }

  void acceptUpstream(StreamTransport &&transport) {
    if (ctx_.stopToken.stop_requested()) {
      return;
    }
    const auto id = utils::genConnectionId();
    LOG_INFO_SYSTEM("New upstream connection id: {}", id);
    if (sessionsMap_.size() >= MAX_CONNECTIONS) {
      LOG_ERROR("Connection limit reached");
      return;
    }
    auto chan = std::make_shared<UpstreamChan>(std::move(transport), id, UpstreamBus{ctx_.bus});
    chan->read();
    unauthorizedUpstreamMap_.insert(std::make_pair(id, std::move(chan)));
  }

  void acceptDownstream(StreamTransport &&transport) {
    if (ctx_.stopToken.stop_requested()) {
      return;
    }
    const auto id = utils::genConnectionId();
    LOG_INFO_SYSTEM("New downstream connection Id: {}", id);
    if (sessionsMap_.size() >= MAX_CONNECTIONS) {
      LOG_ERROR("Connection limit reached");
      return;
    }
    auto chan = std::make_shared<DownstreamChan>(std::move(transport), id, DownstreamBus{ctx_.bus});
    chan->read();
    unauthorizedDownstreamMap_.insert(std::make_pair(id, std::move(chan)));
  }

  void close() {
    LOG_DEBUG_SYSTEM("close");
    for (auto iter = sessionsMap_.begin(); iter != sessionsMap_.end(); ++iter) {
      if (iter->second->upstreamChannel != nullptr) {
        iter->second->upstreamChannel->close();
      }
      if (iter->second->downstreamChannel != nullptr) {
        iter->second->downstreamChannel->close();
      }
    }
    for (auto iter = unauthorizedUpstreamMap_.begin(); iter != unauthorizedUpstreamMap_.end();
         ++iter) {
      if (iter->second != nullptr) {
        iter->second->close();
      }
    }
    for (auto iter = unauthorizedDownstreamMap_.begin(); iter != unauthorizedDownstreamMap_.end();
         ++iter) {
      if (iter->second != nullptr) {
        iter->second->close();
      }
    }
    sessionsMap_.clear();
    unauthorizedUpstreamMap_.clear();
    unauthorizedDownstreamMap_.clear();
  }

private:
  void post(CRef<ServerOrderStatus> status) {
    if (ctx_.stopToken.stop_requested()) {
      return;
    }
    LOG_DEBUG("{}", toString(status));
    const auto sessionIter = sessionsMap_.find(status.clientId);
    if (sessionIter == sessionsMap_.end()) [[unlikely]] {
      LOG_TRACE("Client {} is offline", status.clientId);
      return;
    }
    const auto session = sessionIter->second;
    if (session->downstreamChannel != nullptr) [[likely]] {
      session->downstreamChannel->write(status.orderStatus);
    } else {
      LOG_ERROR("No downstream connection for {}", status.clientId);
    }
  }

  void post(CRef<ServerLoginResponse> loginResult) {
    if (ctx_.stopToken.stop_requested()) {
      return;
    }
    LOG_DEBUG("onLoginResponse {} {}", loginResult.ok, loginResult.clientId);
    const auto channelIter = unauthorizedUpstreamMap_.find(loginResult.connectionId);
    if (channelIter == unauthorizedUpstreamMap_.end()) [[unlikely]] {
      LOG_ERROR_SYSTEM("Connection not found {}", loginResult.connectionId);
      return;
    }
    const auto channel = channelIter->second;
    unauthorizedUpstreamMap_.erase(channelIter->first);

    if (channel == nullptr) [[unlikely]] {
      LOG_ERROR_SYSTEM("Connection not initialized {}", loginResult.connectionId);
      return;
    }
    if (!loginResult.ok) {
      LOG_ERROR_SYSTEM("Authentication failed for {}, closing channel", loginResult.connectionId);
      channel->write(LoginResponse{0, false, loginResult.error});
    } else {
      const auto token = utils::genToken();

      auto newSession = std::make_shared<Session>();
      newSession->clientId = loginResult.clientId;
      newSession->token = token;
      newSession->upstreamChannel = channel;

      if (!sessionsMap_.insert(std::make_pair(loginResult.clientId, newSession)).second) {
        LOG_ERROR_SYSTEM("{} already authorized", loginResult.clientId);
        channel->write(LoginResponse{0, false, "Already authorized"});
      } else {
        LOG_INFO_SYSTEM("{} authenticated", loginResult.clientId);
        newSession->upstreamChannel->write(LoginResponse{token, true});
        newSession->upstreamChannel->authenticate(loginResult.clientId);
      }
    }
  }

  void post(CRef<ServerTokenBindRequest> request) {
    if (ctx_.stopToken.stop_requested()) {
      return;
    }
    LOG_INFO_SYSTEM("Token bind request {} {}", request.connectionId, request.request.token);
    const auto channelIter = unauthorizedDownstreamMap_.find(request.connectionId);
    if (channelIter == unauthorizedDownstreamMap_.end()) {
      LOG_INFO_SYSTEM("Client already disconnected");
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
      LOG_ERROR_SYSTEM("Invalid token received from {}", request.connectionId);
      downstreamChannel->write(LoginResponse{0, false, "Invalid token"});
      return;
    }
    const auto session = sessionIter->second;
    if (session->downstreamChannel == nullptr) {
      session->downstreamChannel = std::move(downstreamChannel);
      session->downstreamChannel->authenticate(session->clientId);
      session->downstreamChannel->write(LoginResponse{request.request.token, true});
      LOG_INFO_SYSTEM("New Session {} {}", sessionIter->first, request.request.token);
    } else {
      LOG_ERROR_SYSTEM("Downstream channel {} is already connected", request.connectionId);
      downstreamChannel->write(LoginResponse{0, false, "Already connected"});
      downstreamChannel->close();
    }
    printStats();
  }

  void post(CRef<ChannelStatusEvent> event) {
    LOG_DEBUG("{}", toString(event));
    switch (event.event.status) {
    case ConnectionStatus::Connected:
      // Nothing to do here for now
      break;
    case ConnectionStatus::Disconnected:
    case ConnectionStatus::Error:
      LOG_DEBUG("Channel {} disconnected", event.event.connectionId);
      // No info about whether it was upstream or downstream event, but all ids are unique
      // so erase both unauthorized containers, and cleanup session if it was started
      if (event.clientId.has_value()) {
        const auto sessionIter = sessionsMap_.find(event.clientId.value());
        if (sessionIter != sessionsMap_.end()) [[unlikely]] {
          const auto session = sessionIter->second;
          session->downstreamChannel->close();
          session->upstreamChannel->close();
          sessionsMap_.erase(event.clientId.value());
          LOG_DEBUG("Client {} disconnected", event.clientId.value());
        }
      }
      unauthorizedUpstreamMap_.erase(event.event.connectionId);
      unauthorizedDownstreamMap_.erase(event.event.connectionId);
      break;
    default:
      break;
    }
  }

  inline void printStats() const { LOG_INFO_SYSTEM("Active sessions: {}", sessionsMap_.size()); }

private:
  Context &ctx_;

  folly::AtomicHashMap<ConnectionId, SPtr<UpstreamChan>> unauthorizedUpstreamMap_;
  folly::AtomicHashMap<ConnectionId, SPtr<DownstreamChan>> unauthorizedDownstreamMap_;

  folly::AtomicHashMap<ClientId, SPtr<Session>> sessionsMap_;
};

} // namespace hft::server

#endif // HFT_SERVER_NETWORKSESSIONMANAGER_HPP
