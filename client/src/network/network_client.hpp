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
#include "client_command.hpp"
#include "client_events.hpp"
#include "client_types.hpp"
#include "config/client_config.hpp"
#include "connection_state.hpp"
#include "domain_types.hpp"
#include "logging.hpp"
#include "network/connection_status.hpp"
#include "network/transport/tcp_transport.hpp"
#include "network/transport/udp_transport.hpp"
#include "types.hpp"
#include "utils/string_utils.hpp"
#include "utils/utils.hpp"

namespace hft::client {

/**
 * @brief Manages all the Tcp and Udp connections to the server, handles authentication
 */
class NetworkClient {
  using ClientTcpTransport = TcpTransport<Bus>;
  using ClientUdpTransport = UdpTransport<Bus>;

public:
  NetworkClient(Bus &bus)
      : guard_{MakeGuard(ioCtx_.get_executor())}, bus_{bus},
        upstreamTransport_{createUpstreamTransport()},
        downstreamTransport_{createDownstreamTransport()},
        pricesTransport_{createPricesTransport()} {
    bus_.marketBus.setHandler<Order>(
        [this](CRef<Order> order) { upstreamTransport_.write(order); });
    bus_.systemBus.subscribe<ConnectionStatusEvent>(
        [this](CRef<ConnectionStatusEvent> event) { onConnectionStatus(event); });
    bus_.systemBus.subscribe<LoginResponse>(
        [this](CRef<LoginResponse> event) { onLoginResponse(event); });
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
      upstreamTransport_.connect();
      downstreamTransport_.connect();
      pricesTransport_.read();
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
      if (upstreamTransport_.isConnected() && downstreamTransport_.isConnected() &&
          state_ == ConnectionState::Disconnected) {
        // Start authentication process, first send credentials over upstream socket
        state_ = ConnectionState::Connected;
        bus_.post(ClientEvent::Connected);
        upstreamTransport_.write(LoginRequest{ClientConfig::cfg.name, ClientConfig::cfg.password});
      }
    } else {
      const auto prevState = state_;
      state_ = ConnectionState::Disconnected;
      token_.reset();
      if (upstreamTransport_.isError() || downstreamTransport_.isError()) {
        upstreamTransport_.close();
        downstreamTransport_.close();
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
      downstreamTransport_.write(TokenBindRequest{event.token});
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
  ClientTcpTransport createUpstreamTransport() {
    return {utils::generateConnectionId(), TcpSocket{ioCtx_},
            TcpEndpoint{Ip::make_address(ClientConfig::cfg.url), ClientConfig::cfg.portTcpUp},
            bus_};
  }

  ClientTcpTransport createDownstreamTransport() {
    return {utils::generateConnectionId(), TcpSocket{ioCtx_},
            TcpEndpoint{Ip::make_address(ClientConfig::cfg.url), ClientConfig::cfg.portTcpDown},
            bus_};
  }

  ClientUdpTransport createPricesTransport() {
    return {utils::generateConnectionId(),
            utils::createUdpSocket(ioCtx_, false, ClientConfig::cfg.portUdp),
            UdpEndpoint(Udp::v4(), ClientConfig::cfg.portUdp), bus_};
  }

private:
  IoCtx ioCtx_;
  ContextGuard guard_;

  Bus &bus_;

  ClientTcpTransport upstreamTransport_;
  ClientTcpTransport downstreamTransport_;
  ClientUdpTransport pricesTransport_;

  ConnectionState state_{ConnectionState::Disconnected};
  Optional<Token> token_;

  std::vector<Thread> workerThreads_;
};

} // namespace hft::client

#endif // HFT_CLIENT_NETWORKCLIENT_HPP
