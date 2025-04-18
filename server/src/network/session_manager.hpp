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
#include "network/size_framer.hpp"
#include "network/socket_status.hpp"
#include "network/transport/tcp_transport.hpp"
#include "server_events.hpp"
#include "server_types.hpp"
#include "session_channel.hpp"
#include "utils/utils.hpp"

namespace hft::server {

/**
 * @brief Manages sessions, accepts messages only from authorized connections
 * @details Login flow is the following:
 * - Client sends credentials via upstream socket
 * - Credentials login request gets posted over the system bus
 * - Postgres adapter handles request and sends response back to the system bus
 * - If auth successfull - session token gets generated and sent back to the client
 * - Client sends token via downstream socket, and session is fully operational
 */
class SessionManager {
  struct Session {
    Token token;
    Opt<ConnectionId> upstreamId;
    Opt<ConnectionId> downstreamId;
  };

  using ChannelMap = folly::AtomicHashMap<ConnectionId, UPtr<SessionChannel>>;
  using SessionMap = folly::AtomicHashMap<ClientId, Session>;

public:
  explicit SessionManager(Bus &bus)
      : bus_{bus}, upstreamMap_{MAX_CONNECTIONS}, downstreamMap_{MAX_CONNECTIONS},
        sessionsMap_{MAX_CONNECTIONS} {
    bus_.marketBus.setHandler<ServerOrderStatus>(
        [this](CRef<ServerOrderStatus> event) { onOrderStatus(event); });
    bus_.systemBus.subscribe<ServerLoginResponse>(
        [this](CRef<ServerLoginResponse> event) { onLoginResponse(event); });
    bus_.systemBus.subscribe<ServerTokenBindRequest>(
        [this](CRef<ServerTokenBindRequest> event) { onTokenBindRequest(event); });
    bus_.systemBus.subscribe<ConnectionStatusEvent>(
        [this](CRef<ConnectionStatusEvent> event) { onConnectionStatusEvent(event); });
  }

  void acceptUpstream(TcpSocket socket) {
    if (upstreamMap_.size() >= MAX_CONNECTIONS) {
      LOG_ERROR("Upstream connection limit reached");
      return;
    }
    const auto id = utils::generateConnectionId();
    auto newTransport = std::make_unique<SessionChannel>(id, std::move(socket), bus_);
    const auto result = upstreamMap_.insert(std::make_pair(id, std::move(newTransport)));
    if (!result.second) {
      LOG_ERROR("Failed to insert new upstream connection");
    } else {
      LOG_INFO_SYSTEM("New upstream connection id:{}", id);
    }
  }

  void acceptDownstream(TcpSocket socket) {
    if (downstreamMap_.size() >= MAX_CONNECTIONS) {
      LOG_ERROR("Downstream connection limit reached");
      return;
    }
    const auto id = utils::generateConnectionId();
    auto newTransport = std::make_unique<SessionChannel>(id, std::move(socket), bus_);
    const auto result = downstreamMap_.insert(std::make_pair(id, std::move(newTransport)));
    if (!result.second) {
      LOG_ERROR("Failed to insert new downstream connection");
    } else {
      LOG_INFO_SYSTEM("New downstream connection Id:{}", id);
    }
  }

private:
  void onOrderStatus(CRef<ServerOrderStatus> status) {
    LOG_DEBUG(utils::toString(status));

    const auto sessionIt = sessionsMap_.find(status.clientId);
    if (sessionIt == sessionsMap_.end() || !sessionIt->second.downstreamId.has_value()) {
      LOG_INFO("Client {} is offline", status.clientId);
    } else {
      const auto downstreamId = sessionIt->second.downstreamId.value();
      const auto channelIter = downstreamMap_.find(downstreamId);
      if (channelIter != downstreamMap_.end()) {
        channelIter->second->write(status.orderStatus);
      } else {
        LOG_ERROR("Downstream socket not found {}", downstreamId);
      }
    }
  }

  void onLoginResponse(CRef<ServerLoginResponse> serverResponse) {
    LOG_DEBUG("onLoginResponse {} {}", serverResponse.ok, serverResponse.token);
    const auto channelIter = upstreamMap_.find(serverResponse.connectionId);
    if (channelIter == upstreamMap_.end()) {
      LOG_ERROR("Connection not found {}", serverResponse.connectionId);
      return;
    }
    LoginResponse response;
    if (!serverResponse.ok) {
      LOG_ERROR("Authentication failed for {}, closing channel", serverResponse.connectionId);
      upstreamMap_.erase(channelIter->first);
      response.error = serverResponse.error;
    } else {
      const auto token = utils::generateToken();

      Session newSession;
      newSession.token = token;
      newSession.upstreamId = channelIter->first;
      const auto result =
          sessionsMap_.insert(std::make_pair(serverResponse.clientId, std::move(newSession)));
      if (!result.second) {
        LOG_ERROR("Failed to insert new session");
        upstreamMap_.erase(channelIter->first);
        response.error = "Internal error";
      } else {
        response.token = token;
        response.ok = true;
      }
    }
    channelIter->second->write(response);
  }

  void onTokenBindRequest(CRef<ServerTokenBindRequest> request) {
    LOG_DEBUG("Token bind request {} {}", request.connectionId, request.request.token);

    LoginResponse response;
    const auto channelIter = downstreamMap_.find(request.connectionId);
    if (channelIter == downstreamMap_.end()) {
      LOG_WARN("Client already disconnected");
      return;
    }
    const auto sessionIter =
        std::find_if(sessionsMap_.begin(), sessionsMap_.end(),
                     [token = request.request.token](const SessionMap::value_type &session) {
                       return session.second.token == token;
                     });
    if (sessionIter == sessionsMap_.end()) {
      LOG_ERROR("Invalid token received from {}", request.connectionId);
      response.error = "Invalid token";
    } else if (sessionIter->second.downstreamId.has_value()) {
      LOG_ERROR("Downstream channel {} is already authenticated", request.connectionId);
      response.error = "Already authenticated";
    } else {
      sessionIter->second.downstreamId = request.connectionId;
      response.ok = true;
      response.token = request.request.token;
    }

    channelIter->second->write(response);
    LOG_INFO_SYSTEM("Session started clientId: {} Token: {}, UpId: {}, DownId: {}",
                    sessionIter->first, request.request.token,
                    sessionIter->second.upstreamId.value_or(0),
                    sessionIter->second.downstreamId.value_or(0));
    printStats();
  }

  void onConnectionStatusEvent(CRef<ConnectionStatusEvent> event) {
    LOG_TRACE_SYSTEM(utils::toString(event));
    if (event.status == ConnectionStatus::Connected) {
      return;
    }
    Opt<ClientId> clientId;
    // Cleanup disconnected channel from connections map and session map
    // Event could be of either up or downstream channel
    const auto upstreamIter = upstreamMap_.find(event.connectionId);
    if (upstreamIter != upstreamMap_.end()) {
      clientId = upstreamIter->second->clientId();
      upstreamMap_.erase(upstreamIter->first);
    }
    const auto downstreamIter = downstreamMap_.find(event.connectionId);
    if (downstreamIter != downstreamMap_.end()) {
      clientId = downstreamIter->second->clientId();
      downstreamMap_.erase(downstreamIter->first);
    }
    // If session was already started - clean it up
    if (clientId.has_value()) {
      const auto sessionIter = sessionsMap_.find(clientId.value());
      if (sessionIter->second.downstreamId.has_value()) {
        downstreamMap_.erase(sessionIter->second.downstreamId.value());
      }
      if (sessionIter->second.upstreamId.has_value()) {
        upstreamMap_.erase(sessionIter->second.upstreamId.value());
      }
      sessionsMap_.erase(clientId.value());
      LOG_INFO_SYSTEM("{} disconnected", clientId.value());
      printStats();
    }
  }

  inline void printStats() const { LOG_INFO_SYSTEM("Active sessions: {}", sessionsMap_.size()); }

private:
  Bus &bus_;

  ChannelMap upstreamMap_;
  ChannelMap downstreamMap_;
  SessionMap sessionsMap_;
};

} // namespace hft::server

#endif // HFT_SERVER_SESSIONMANAGER_HPP