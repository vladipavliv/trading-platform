/**
 * @author Vladimir Pavliv
 * @date 2025-04-06
 */

#ifndef HFT_CLIENT_CLIENTCONNECTIONMANAGER_HPP
#define HFT_CLIENT_CLIENTCONNECTIONMANAGER_HPP

#include "client_connection_state.hpp"
#include "client_events.hpp"
#include "client_types.hpp"
#include "config/client_config.hpp"
#include "network/channel.hpp"
#include "network/connection_status.hpp"
#include "types.hpp"
#include "utils/utils.hpp"

namespace hft::client {

template <typename NetworkClient>
class ClientConnectionManager {
  using StreamTransport = NetworkClient::StreamTransport;
  using DatagramTransport = NetworkClient::DatagramTransport;

  using StreamChannel = Channel<StreamTransport, ClientBus>;
  using DatagramChannel = Channel<DatagramTransport, ClientBus>;

public:
  ClientConnectionManager(ClientBus &bus, NetworkClient &networkClient)
      : bus_{bus}, networkClient_{networkClient} {
    networkClient_.setUpstreamClb(
        [this](StreamTransport &&transport) { onUpstreamConnected(std::move(transport)); });
    networkClient_.setDownstreamClb(
        [this](StreamTransport &&transport) { onDownstreamConnected(std::move(transport)); });
    networkClient_.setDatagramClb(
        [this](DatagramTransport &&transport) { onDatagramConnected(std::move(transport)); });

    bus_.subscribe<Order>([this](CRef<Order> order) {
      if (upstreamChannel_) {
        upstreamChannel_->write(order);
      }
    });
    bus_.subscribe<ConnectionStatusEvent>(
        [this](CRef<ConnectionStatusEvent> event) { onConnectionStatus(event); });
    bus_.subscribe<LoginResponse>([this](CRef<LoginResponse> event) { onLoginResponse(event); });
  }

private:
  void onUpstreamConnected(StreamTransport &&transport) {
    const size_t id = utils::generateConnectionId();
    upstreamChannel_ = std::make_unique<StreamChannel>(std::move(transport), id, bus_);
    upstreamChannel_->write(LoginRequest{ClientConfig::cfg.name, ClientConfig::cfg.password});
  }

  void onDownstreamConnected(StreamTransport &&transport) {
    const size_t id = utils::generateConnectionId();
    downstreamChannel_ = std::make_unique<StreamChannel>(std::move(transport), id, bus_);
  }

  void onDatagramConnected(DatagramTransport &&transport) {
    const size_t id = utils::generateConnectionId();
    pricesChannel_ = std::make_unique<DatagramChannel>(std::move(transport), id, bus_);
  }

  void onConnectionStatus(CRef<ConnectionStatusEvent> event) {
    LOG_DEBUG("{}", utils::toString(event));
    if (event.status != ConnectionStatus::Connected) {
      reset();
    }
  }

  void onLoginResponse(CRef<LoginResponse> event) {
    LOG_DEBUG("{}", utils::toString(event));
    if (event.ok) {
      token_ = event.token;
    } else {
      LOG_ERROR_SYSTEM("Login failed {}", event.error);
      reset();
      return;
    }
    switch (state_) {
    case ConnectionState::Connected: {
      LOG_INFO_SYSTEM("Login successfull, token: {}", event.token);
      // Now authenticate downstream socket by sending token
      state_ = ConnectionState::TokenReceived;
      downstreamChannel_->write(TokenBindRequest{event.token});
      break;
    }
    case ConnectionState::TokenReceived: {
      LOG_INFO_SYSTEM("Authenticated");
      state_ = ConnectionState::Authenticated;
      bus_.post(ClientState::Connected);
      break;
    }
    default:
      break;
    }
  }

  void reset() {
    token_.reset();
    upstreamChannel_.reset();
    downstreamChannel_.reset();
    pricesChannel_.reset();
    state_ = ConnectionState::Disconnected;
    bus_.post(ClientState::Disconnected);
  }

private:
  ClientBus &bus_;

  NetworkClient &networkClient_;

  UPtr<StreamChannel> upstreamChannel_;
  UPtr<StreamChannel> downstreamChannel_;
  UPtr<DatagramChannel> pricesChannel_;

  Optional<Token> token_;
  ConnectionState state_{ConnectionState::Disconnected};
};
} // namespace hft::client

#endif // HFT_CLIENT_CLIENTCONNECTIONMANAGER_HPP