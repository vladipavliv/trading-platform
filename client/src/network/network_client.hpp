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
#include "client_types.hpp"
#include "config/client_config.hpp"
#include "connection_state.hpp"
#include "domain_types.hpp"
#include "logging.hpp"
#include "network/socket_status.hpp"
#include "network/transport/tcp_transport.hpp"
#include "network/transport/udp_transport.hpp"

#include "client_command.hpp"
#include "client_events.hpp"
#include "types.hpp"
#include "utils/utils.hpp"

namespace hft::client {

/**
 * @brief Manages all the Tcp and Udp connections to the server, handles authentication
 */
class NetworkClient {
  using ClientTcpTransport = TcpTransport<NetworkClient>;
  using ClientUdpTransport = UdpTransport<Bus>;

public:
  NetworkClient(Bus &bus)
      : guard_{MakeGuard(ioCtx_.get_executor())}, bus_{bus},
        upstreamTransport_{createUpstreamTransport()},
        downstreamTransport_{createDownstreamTransport()},
        pricesTransport_{createPricesTransport()} {
    bus_.marketBus.setHandler<Order>([this](CRef<Order> order) {
      if (state_ != ConnectionState::AuthenticatedDownstream || !token_.has_value()) {
        LOG_ERROR_SYSTEM("Wrong state {}", utils::toString(state_));
        return;
      }
      upstreamTransport_.write(order);
    });
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
    for (int i = 0; i < cores; ++i) {
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
      state_ = ConnectionState::Connecting;
      upstreamTransport_.connect();
      downstreamTransport_.connect();
      pricesTransport_.read();
    });
  }

  template <typename MessageType>
  void post(CRef<MessageType> message) {
    LOG_DEBUG(utils::toString(message));
    bus_.post(message);
  }

  void post(CRef<ConnectionStatusEvent> event) {
    LOG_DEBUG(utils::toString(event));
    if (event.status == ConnectionStatus::Connected) {
      if (upstreamTransport_.status() == ConnectionStatus::Connected &&
          downstreamTransport_.status() == ConnectionStatus::Connected &&
          state_ == ConnectionState::Connecting) {
        LOG_DEBUG_SYSTEM("Connected to the server");
        // Start authentication process, first send credentials over upstream socket
        state_ = ConnectionState::Connected;
        upstreamTransport_.write(LoginRequest{ClientConfig::cfg.name, ClientConfig::cfg.password});
      }
    } else {
      const auto prevState = state_;
      state_ = ConnectionState::Disconnected;
      token_.reset();
      upstreamTransport_.close();
      downstreamTransport_.close();
      if (prevState != ConnectionState::Disconnected) {
        bus_.post(ClientEvent::DisconnectedFromTheServer);
      }
    }
  }

  void post(CRef<LoginResponse> event) {
    LOG_DEBUG(utils::toString(event));
    if (event.ok) {
      token_ = event.token;
    } else {
      LOG_ERROR_SYSTEM("Login failed");
      return;
    }
    switch (state_) {
    case ConnectionState::Connected: {
      LOG_DEBUG_SYSTEM("Authenticated upstream");
      // Now authenticate downstream socket by sending token
      downstreamTransport_.write(TokenBindRequest{event.token});
      state_ = ConnectionState::AuthenticatedUpstream;
      break;
    }
    case ConnectionState::AuthenticatedUpstream: {
      LOG_DEBUG_SYSTEM("Authenticated downstream");
      state_ = ConnectionState::AuthenticatedDownstream;
      bus_.post(ClientEvent::ConnectedToTheServer);
      break;
    }
    default:
      break;
    }
  }

private:
  ClientTcpTransport createUpstreamTransport() {
    return {TcpSocket{ioCtx_},
            TcpEndpoint{Ip::make_address(ClientConfig::cfg.url), ClientConfig::cfg.portTcpUp},
            *this};
  }

  ClientTcpTransport createDownstreamTransport() {
    return {TcpSocket{ioCtx_},
            TcpEndpoint{Ip::make_address(ClientConfig::cfg.url), ClientConfig::cfg.portTcpDown},
            *this};
  }

  ClientUdpTransport createPricesTransport() {
    return {utils::createUdpSocket(ioCtx_, false, ClientConfig::cfg.portUdp),
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
  Opt<Token> token_;

  std::vector<Thread> workerThreads_;
};

} // namespace hft::client

#endif // HFT_CLIENT_NETWORKCLIENT_HPP
