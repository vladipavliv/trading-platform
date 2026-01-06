/**
 * @author Vladimir Pavliv
 * @date 2025-03-29
 */

#ifndef HFT_SERVER_SESSIONMANAGER_HPP
#define HFT_SERVER_SESSIONMANAGER_HPP

#include <boost/unordered/unordered_flat_map.hpp>

#include "boost_types.hpp"
#include "constants.hpp"
#include "logging.hpp"
#include "network/async_transport.hpp"
#include "network/connection_status.hpp"
#include "network/session_channel.hpp"
#include "server_events.hpp"
#include "server_types.hpp"
#include "utils/string_utils.hpp"
#include "utils/utils.hpp"

namespace hft::server {

/**
 * @brief Manages sessions, generates tokens, authenticates channels
 */
template <AsyncTransport T>
class SessionManager {
  using Chan = SessionChannel<T>;

  /**
   * @brief Client session info
   * @todo Make rate limiting counter
   */
  struct Session {
    ClientId clientId;
    Token token;
    SPtr<Chan> upstreamChannel;
    SPtr<Chan> downstreamChannel;
  };

public:
  using Transport = T;
  using DrainHook = std::function<void(Callback &&)>;

  explicit SessionManager(ServerBus &bus) : bus_{bus} {
    bus_.subscribe<ServerOrderStatus>(
        [this](CRef<ServerOrderStatus> event) { onOrderStatus(event); });
    bus_.subscribe<ServerLoginResponse>(
        [this](CRef<ServerLoginResponse> event) { onLoginResponse(event); });
    bus_.subscribe<ServerTokenBindRequest>(
        [this](CRef<ServerTokenBindRequest> event) { onTokenBindRequest(event); });
    bus_.subscribe<ChannelStatusEvent>(
        [this](CRef<ChannelStatusEvent> event) { onChannelStatus(event); });
  }

  void setDrainHook(DrainHook &&drainHook) { drainHook_ = std::move(drainHook); }

  void acceptUpstream(Transport &&transport) {
    const auto id = utils::generateConnectionId();
    LOG_INFO_SYSTEM("New upstream connection id: {}", id);
    if (sessionsMap_.size() >= MAX_CONNECTIONS) {
      LOG_ERROR("Connection limit reached");
      return;
    }
    auto chan = std::make_shared<Chan>(std::move(transport), id, bus_);
    chan->read();
    unauthorizedUpstreamMap_.insert(std::make_pair(id, std::move(chan)));
  }

  void acceptDownstream(Transport &&transport) {
    const auto id = utils::generateConnectionId();
    LOG_INFO_SYSTEM("New downstream connection Id: {}", id);
    if (sessionsMap_.size() >= MAX_CONNECTIONS) {
      LOG_ERROR("Connection limit reached");
      return;
    }
    auto chan = std::make_shared<Chan>(std::move(transport), id, bus_);
    chan->read();
    unauthorizedDownstreamMap_.insert(std::make_pair(id, std::move(chan)));
  }

  void close() {
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
  void onOrderStatus(CRef<ServerOrderStatus> status) {
    size_t iterations = 0;
    while (!statusQueue_.push(status)) {
      asm volatile("pause" ::: "memory");
      if (++iterations > BUSY_WAIT_CYCLES) {
        throw std::runtime_error("Status queue is full");
      }
    }
    if (!drainScheduled_.exchange(true, std::memory_order_acquire)) {
      drainHook_([this]() { drainStatusQueue(); });
    }
  }

  void onLoginResponse(CRef<ServerLoginResponse> loginResult) {
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
      const auto token = utils::generateToken();

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

  void onTokenBindRequest(CRef<ServerTokenBindRequest> request) {
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
      session->upstreamChannel->read();
      LOG_INFO_SYSTEM("New Session {} {}", sessionIter->first, request.request.token);
    } else {
      LOG_ERROR_SYSTEM("Downstream channel {} is already connected", request.connectionId);
      downstreamChannel->write(LoginResponse{0, false, "Already connected"});
      downstreamChannel->close();
    }
    printStats();
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

  inline void drainStatusQueue() {
    ServerOrderStatus status;
    while (statusQueue_.pop(status)) {
      processStatus(status);
    }
    drainScheduled_.store(false, std::memory_order_release);
    if (!statusQueue_.empty()) {
      if (!drainScheduled_.exchange(true, std::memory_order_acquire)) {
        drainHook_([this]() { drainStatusQueue(); });
      }
    }
  }

  inline void processStatus(CRef<ServerOrderStatus> status) {
    LOG_DEBUG("{}", utils::toString(status));
    const auto sessionIter = sessionsMap_.find(status.clientId);
    if (sessionIter == sessionsMap_.end()) [[unlikely]] {
      LOG_ERROR_SYSTEM("Client {} is offline", status.clientId);
      return;
    }
    const auto session = sessionIter->second;
    if (session->downstreamChannel != nullptr) [[likely]] {
      session->downstreamChannel->write(status.orderStatus);
    } else {
      LOG_ERROR_SYSTEM("No downstream connection for {}", status.clientId);
    }
  }

  inline void printStats() const { LOG_INFO_SYSTEM("Active sessions: {}", sessionsMap_.size()); }

private:
  ServerBus &bus_;

  boost::unordered_flat_map<ConnectionId, SPtr<Chan>> unauthorizedUpstreamMap_;
  boost::unordered_flat_map<ConnectionId, SPtr<Chan>> unauthorizedDownstreamMap_;

  boost::unordered_flat_map<ClientId, SPtr<Session>> sessionsMap_;

  DrainHook drainHook_;
  std::atomic_bool drainScheduled_{false};
  VyukovQueue<ServerOrderStatus> statusQueue_;
};

} // namespace hft::server

#endif // HFT_SERVER_SESSIONMANAGER_HPP
