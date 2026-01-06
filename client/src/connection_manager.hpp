/**
 * @author Vladimir Pavliv
 * @date 2025-04-06
 */

#ifndef HFT_CLIENT_CONNECTIONMANAGER_HPP
#define HFT_CLIENT_CONNECTIONMANAGER_HPP

#include "bus/bus_hub.hpp"
#include "bus/bus_restrictor.hpp"
#include "config/client_config.hpp"
#include "connection_state.hpp"
#include "events.hpp"
#include "network/channel.hpp"
#include "network/connection_status.hpp"
#include "traits.hpp"
#include "types.hpp"
#include "utils/utils.hpp"

namespace hft::client {

class ConnectionManager {
  using UpStreamChannel = Channel<StreamTransport, UpstreamBus>;
  using DownStreamChannel = Channel<StreamTransport, DownstreamBus>;
  using DatagramChannel = Channel<DatagramTransport, DatagramBus>;

public:
  ConnectionManager(ClientBus &bus, NetworkClient &networkClient)
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
    bus_.subscribe<ConnectionStatusEvent>( // format
        [this](CRef<ConnectionStatusEvent> event) { onConnectionStatus(event); });
    bus_.subscribe<LoginResponse>( // format
        [this](CRef<LoginResponse> event) { onLoginResponse(event); });
  }

  void close() { reset(); }

private:
  void onUpstreamConnected(StreamTransport &&transport) {
    if (upstreamChannel_) {
      LOG_ERROR_SYSTEM("Already connected upstream");
      return;
    }
    const size_t id = utils::generateConnectionId();
    upstreamChannel_ =
        std::make_shared<UpStreamChannel>(std::move(transport), id, UpstreamBus{bus_});
    upstreamChannel_->read();
    tryAuthenticate();
  }

  void onDownstreamConnected(StreamTransport &&transport) {
    if (downstreamChannel_) {
      LOG_ERROR_SYSTEM("Already connected downstream");
      return;
    }
    const size_t id = utils::generateConnectionId();
    downstreamChannel_ =
        std::make_shared<DownStreamChannel>(std::move(transport), id, DownstreamBus{bus_});
    downstreamChannel_->read();
    tryAuthenticate();
  }

  void onDatagramConnected(DatagramTransport &&transport) {
    const size_t id = utils::generateConnectionId();
    pricesChannel_ = std::make_shared<DatagramChannel>(std::move(transport), id, DatagramBus{bus_});
    pricesChannel_->read();
  }

  void onConnectionStatus(CRef<ConnectionStatusEvent> event) {
    LOG_DEBUG("{}", utils::toString(event));
    if (event.status != ConnectionStatus::Connected) {
      reset();
    }
  }

  void tryAuthenticate() {
    if (upstreamChannel_ != nullptr && downstreamChannel_ != nullptr) {
      LOG_INFO_SYSTEM("Authenticating");
      state_ = ConnectionState::Connected;
      upstreamChannel_->write(LoginRequest{ClientConfig::cfg.name, ClientConfig::cfg.password});
    }
  }

  void onLoginResponse(CRef<LoginResponse> event) {
    LOG_INFO_SYSTEM("{}", utils::toString(event));
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

  SPtr<UpStreamChannel> upstreamChannel_;
  SPtr<DownStreamChannel> downstreamChannel_;
  SPtr<DatagramChannel> pricesChannel_;

  Optional<Token> token_;
  ConnectionState state_{ConnectionState::Disconnected};
};
} // namespace hft::client

#endif // HFT_CLIENT_CONNECTIONMANAGER_HPP