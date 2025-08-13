/**
 * @author Vladimir Pavliv
 * @date 2025-02-13
 */

#ifndef HFT_CLIENT_NETWORKCLIENT_HPP
#define HFT_CLIENT_NETWORKCLIENT_HPP

#include <format>
#include <memory>
#include <vector>

#include "boost_types.hpp"
#include "client_events.hpp"
#include "client_types.hpp"
#include "commands/client_command.hpp"
#include "config/client_config.hpp"
#include "connection_state.hpp"
#include "domain_types.hpp"
#include "logging.hpp"
#include "network/channels/tcp_channel.hpp"
#include "network/channels/udp_channel.hpp"
#include "network/connection_status.hpp"
#include "types.hpp"
#include "utils/string_utils.hpp"
#include "utils/utils.hpp"

namespace hft::client {

/**
 * @brief Manages all the Tcp and Udp connections to the server, handles authentication
 */
class NetworkClient {
  using ClientTcpChannel = TcpChannel<ClientBus>;
  using ClientUdpChannel = UdpChannel<ClientBus>;

public:
  NetworkClient(ClientBus &bus)
      : guard_{MakeGuard(ioCtx_.get_executor())}, bus_{bus},
        upstreamChannel_{createUpstreamChannel()}, downstreamChannel_{createDownstreamChannel()},
        pricesChannel_{createPricesChannel()} {
    bus_.subscribe<Order>([this](CRef<Order> order) { upstreamChannel_.write(order); });
    bus_.subscribe<ConnectionStatusEvent>(
        [this](CRef<ConnectionStatusEvent> event) { onConnectionStatus(event); });
    bus_.subscribe<LoginResponse>([this](CRef<LoginResponse> event) { onLoginResponse(event); });
  }

  ~NetworkClient() { stop(); }

  void start() {
    auto addThread = [this](uint8_t workerId, bool pinToCore, CoreId coreId = 0) {
      workerThreads_.emplace_back([this, workerId, pinToCore, coreId]() {
        try {
          utils::setTheadRealTime();
          if (pinToCore) {
            utils::pinThreadToCore(coreId);
            LOG_DEBUG("Worker {} started on the core {}", workerId, coreId);
          } else {
            LOG_DEBUG("Worker {} started", workerId);
          }
          ioCtx_.run();
        } catch (const std::exception &e) {
          LOG_ERROR_SYSTEM("Exception in network thread {}", e.what());
          ioCtx_.stop();
        }
      });
    };
    const auto cores = ClientConfig::cfg.coresNetwork.size();
    workerThreads_.reserve(cores == 0 ? 1 : cores);
    if (cores == 0) {
      addThread(0, false);
    }
    for (size_t i = 0; i < cores; ++i) {
      addThread(i, true, ClientConfig::cfg.coresNetwork[i]);
    }
  }

  void stop() { ioCtx_.stop(); }

  void connect() {
    if (state_ != ConnectionState::Disconnected) {
      LOG_ERROR("Wrong state {}", utils::toString(state_));
      return;
    }
    ioCtx_.post([this]() {
      LOG_DEBUG("Connecting to the server");
      upstreamChannel_.connect();
      downstreamChannel_.connect();
      pricesChannel_.read();
    });
  }

  template <typename MessageType>
  void post(CRef<MessageType> message) {
    LOG_DEBUG("{}", utils::toString(message));
    bus_.post(message);
  }

private:
  void onConnectionStatus(CRef<ConnectionStatusEvent> event) {
    LOG_DEBUG("{}", utils::toString(event));
    if (event.status == ConnectionStatus::Connected) {
      if (upstreamChannel_.isConnected() && downstreamChannel_.isConnected() &&
          state_ == ConnectionState::Disconnected) {
        // Start authentication process, first send credentials over upstream socket
        state_ = ConnectionState::Connected;
        bus_.post(ClientEvent::Connected);
        upstreamChannel_.write(LoginRequest{ClientConfig::cfg.name, ClientConfig::cfg.password});
      }
    } else {
      const auto prevState = state_;
      state_ = ConnectionState::Disconnected;
      token_.reset();
      if (upstreamChannel_.isError() || downstreamChannel_.isError()) {
        upstreamChannel_.close();
        downstreamChannel_.close();
        if (prevState != ConnectionState::Disconnected) {
          bus_.post(ClientEvent::Disconnected);
        } else {
          bus_.post(ClientEvent::ConnectionFailed);
        }
      }
    }
  }

  void onLoginResponse(CRef<LoginResponse> event) {
    LOG_DEBUG("{}", utils::toString(event));
    if (event.ok) {
      token_ = event.token;
    } else {
      LOG_ERROR_SYSTEM("Login failed {}", event.error);
      state_ = ConnectionState::Disconnected;
      return;
    }
    switch (state_) {
    case ConnectionState::Connected: {
      LOG_INFO_SYSTEM("Login successfull, token: {}", event.token);
      // Now authenticate downstream socket by sending token
      state_ = ConnectionState::TokenReceived;
      downstreamChannel_.write(TokenBindRequest{event.token});
      break;
    }
    case ConnectionState::TokenReceived: {
      LOG_INFO_SYSTEM("Authenticated");
      bus_.post(ClientEvent::Connected);
      state_ = ConnectionState::Authenticated;
      break;
    }
    default:
      break;
    }
  }

private:
  ClientTcpChannel createUpstreamChannel() {
    return {utils::generateConnectionId(), TcpSocket{ioCtx_},
            TcpEndpoint{Ip::make_address(ClientConfig::cfg.url), ClientConfig::cfg.portTcpUp},
            bus_};
  }

  ClientTcpChannel createDownstreamChannel() {
    return {utils::generateConnectionId(), TcpSocket{ioCtx_},
            TcpEndpoint{Ip::make_address(ClientConfig::cfg.url), ClientConfig::cfg.portTcpDown},
            bus_};
  }

  ClientUdpChannel createPricesChannel() {
    return {utils::generateConnectionId(),
            utils::createUdpSocket(ioCtx_, false, ClientConfig::cfg.portUdp),
            UdpEndpoint(Udp::v4(), ClientConfig::cfg.portUdp), bus_};
  }

private:
  IoCtx ioCtx_;
  ContextGuard guard_;

  ClientBus &bus_;

  ClientTcpChannel upstreamChannel_;
  ClientTcpChannel downstreamChannel_;
  ClientUdpChannel pricesChannel_;

  ConnectionState state_{ConnectionState::Disconnected};
  Optional<Token> token_;

  std::vector<Thread> workerThreads_;
};

} // namespace hft::client

#endif // HFT_CLIENT_NETWORKCLIENT_HPP
