/**
 * @author Vladimir Pavliv
 * @date 2026-01-07
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
#include "primitive_types.hpp"
#include "traits.hpp"
#include "utils/id_utils.hpp"
#include "utils/string_utils.hpp"

namespace hft::client {

class TrustedConnectionManager {
  using TrustedUpstreamBus = BusRestrictor< // format
      ClientBus, ChannelStatusEvent, ConnectionStatusEvent>;
  using TrustedDownstreamBus = // format
      BusRestrictor<ClientBus, OrderStatus, ChannelStatusEvent, ConnectionStatusEvent>;
  using TrustedDatagramBus = // format
      BusRestrictor<ClientBus, TickerPrice, ChannelStatusEvent, ConnectionStatusEvent>;

  using UpStreamChannel = Channel<ShmTransport, TrustedUpstreamBus>;
  using DownStreamChannel = Channel<ShmTransport, TrustedDownstreamBus>;
  using DatagramChannel = Channel<ShmTransport, TrustedDatagramBus>;

public:
  TrustedConnectionManager(ClientBus &bus, ShmClient &networkClient)
      : bus_{bus}, networkClient_{networkClient} {
    LOG_INFO_SYSTEM("TrustedConnectionManager initialized");
    networkClient_.setUpstreamClb(
        [this](ShmTransport &&transport) { onUpstreamConnected(std::move(transport)); });
    networkClient_.setDownstreamClb(
        [this](ShmTransport &&transport) { onDownstreamConnected(std::move(transport)); });
    networkClient_.setDatagramClb(
        [this](ShmTransport &&transport) { onDatagramConnected(std::move(transport)); });

    bus_.subscribe<Order>([this](CRef<Order> order) {
      if (upstreamChannel_) {
        upstreamChannel_->write(order);
      }
    });
  }

  void close() { reset(); }

private:
  void onUpstreamConnected(StreamTransport &&transport) {
    if (upstreamChannel_) {
      LOG_ERROR_SYSTEM("Already connected upstream");
      return;
    }
    LOG_INFO_SYSTEM("Connected upstream");
    const size_t id = utils::generateConnectionId();
    upstreamChannel_ =
        std::make_shared<UpStreamChannel>(std::move(transport), id, TrustedUpstreamBus{bus_});
    upstreamChannel_->read();
    notify();
  }

  void onDownstreamConnected(StreamTransport &&transport) {
    if (downstreamChannel_) {
      LOG_ERROR_SYSTEM("Already connected downstream");
      return;
    }
    LOG_INFO_SYSTEM("Connected downstream");
    const size_t id = utils::generateConnectionId();
    downstreamChannel_ =
        std::make_shared<DownStreamChannel>(std::move(transport), id, TrustedDownstreamBus{bus_});
    downstreamChannel_->read();
    notify();
  }

  void onDatagramConnected(DatagramTransport &&transport) {
    if (pricesChannel_) {
      LOG_ERROR_SYSTEM("Already connected datagram");
      return;
    }
    LOG_INFO_SYSTEM("Connected datagram");
    const size_t id = utils::generateConnectionId();
    pricesChannel_ =
        std::make_shared<DatagramChannel>(std::move(transport), id, TrustedDatagramBus{bus_});
    pricesChannel_->read();
  }

  void onConnectionStatus(CRef<ConnectionStatusEvent> event) {
    LOG_DEBUG("{}", toString(event));
    if (event.status != ConnectionStatus::Connected) {
      reset();
    }
  }

  void notify() {
    if (upstreamChannel_ != nullptr && downstreamChannel_ != nullptr) {
      bus_.post(ClientState::Connected);
    }
  }

  void reset() {
    LOG_DEBUG("TrustedConnectionManager reset");
    upstreamChannel_.reset();
    downstreamChannel_.reset();
    pricesChannel_.reset();
    bus_.post(ClientState::Disconnected);
  }

private:
  ClientBus &bus_;

  ShmClient &networkClient_;

  SPtr<UpStreamChannel> upstreamChannel_;
  SPtr<DownStreamChannel> downstreamChannel_;
  SPtr<DatagramChannel> pricesChannel_;
};
} // namespace hft::client

#endif // HFT_CLIENT_CONNECTIONMANAGER_HPP