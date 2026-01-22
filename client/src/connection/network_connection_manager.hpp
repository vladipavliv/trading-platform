/**
 * @author Vladimir Pavliv
 * @date 2025-04-06
 */

#ifndef HFT_CLIENT_NETWORKCONNECTIONMANAGER_HPP
#define HFT_CLIENT_NETWORKCONNECTIONMANAGER_HPP

#include "bus/bus_hub.hpp"
#include "bus/bus_restrictor.hpp"
#include "config/client_config.hpp"
#include "connection_state.hpp"
#include "events.hpp"
#include "primitive_types.hpp"
#include "traits.hpp"
#include "transport/channel.hpp"
#include "transport/connection_status.hpp"
#include "types/functional_types.hpp"
#include "utils/id_utils.hpp"
#include "utils/string_utils.hpp"

namespace hft::client {

class NetworkConnectionManager {
  using SelfT = NetworkConnectionManager;

  using StreamTHandler = MoveHandler<StreamTransport>;
  using DatagramTHandler = MoveHandler<DatagramTransport>;

  using UpStreamChannel = Channel<StreamTransport, UpstreamBus>;
  using DownStreamChannel = Channel<StreamTransport, DownstreamBus>;
  using DatagramChannel = Channel<DatagramTransport, DatagramBus>;

public:
  NetworkConnectionManager(Context &ctx, IpcClient &ipcClient) : ctx_{ctx}, ipcClient_{ipcClient} {
    LOG_INFO_SYSTEM("NetworkConnectionManager initialized");
    ipcClient_.setUpstreamClb(StreamTHandler::bind<SelfT, &SelfT::onUpstream>(this));
    ipcClient_.setDownstreamClb(StreamTHandler::bind<SelfT, &SelfT::onDownstream>(this));
    ipcClient_.setDatagramClb(DatagramTHandler::bind<SelfT, &SelfT::onDatagram>(this));

    ctx_.bus.subscribe(CRefHandler<Order>::bind<SelfT, &SelfT::post>(this));
    ctx_.bus.subscribe(CRefHandler<LoginResponse>::bind<SelfT, &SelfT::post>(this));
    ctx_.bus.subscribe(CRefHandler<ConnectionStatusEvent>::bind<SelfT, &SelfT::post>(this));
  }

  void close() { reset(); }

private:
  void onUpstream(StreamTransport &&transport) {
    if (upstreamChannel_) {
      LOG_ERROR_SYSTEM("Already connected upstream");
      return;
    }
    LOG_INFO_SYSTEM("Connected upstream");
    const auto id = utils::genConnectionId();
    upstreamChannel_ =
        std::make_shared<UpStreamChannel>(std::move(transport), id, UpstreamBus{ctx_.bus});
    upstreamChannel_->read();
    tryAuthenticate();
  }

  void onDownstream(StreamTransport &&transport) {
    if (downstreamChannel_) {
      LOG_ERROR_SYSTEM("Already connected downstream");
      return;
    }
    LOG_INFO_SYSTEM("Connected downstream");
    const auto id = utils::genConnectionId();
    downstreamChannel_ =
        std::make_shared<DownStreamChannel>(std::move(transport), id, DownstreamBus{ctx_.bus});
    downstreamChannel_->read();
    tryAuthenticate();
  }

  void onDatagram(DatagramTransport &&transport) {
    const auto id = utils::genConnectionId();
    pricesChannel_ =
        std::make_shared<DatagramChannel>(std::move(transport), id, DatagramBus{ctx_.bus});
    pricesChannel_->read();
  }

  void post(CRef<ConnectionStatusEvent> event) {
    LOG_DEBUG("{}", toString(event));
    if (event.status != ConnectionStatus::Connected) {
      reset();
    }
  }

  void tryAuthenticate() {
    if (upstreamChannel_ != nullptr && downstreamChannel_ != nullptr) {
      LOG_INFO_SYSTEM("Authenticating");
      state_ = ConnectionState::Connected;
      upstreamChannel_->write(LoginRequest{ctx_.config.name, ctx_.config.password});
    }
  }

  void post(CRef<LoginResponse> event) {
    LOG_INFO_SYSTEM("{}", toString(event));
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
      ctx_.bus.post(ClientState::Connected);
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
    ctx_.bus.post(ClientState::Disconnected);
  }

  void post(CRef<Order> order) {
    if (upstreamChannel_) {
      upstreamChannel_->write(order);
    }
  }

private:
  Context &ctx_;

  IpcClient &ipcClient_;

  SPtr<UpStreamChannel> upstreamChannel_;
  SPtr<DownStreamChannel> downstreamChannel_;
  SPtr<DatagramChannel> pricesChannel_;

  Optional<Token> token_;
  ConnectionState state_{ConnectionState::Disconnected};
};
} // namespace hft::client

#endif // HFT_CLIENT_NETWORKCONNECTIONMANAGER_HPP