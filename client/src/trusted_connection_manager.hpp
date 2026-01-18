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
#include "primitive_types.hpp"
#include "traits.hpp"
#include "transport/channel.hpp"
#include "transport/connection_status.hpp"
#include "utils/string_utils.hpp"

namespace hft::client {

class TrustedConnectionManager {
  using UpStreamChannel = StreamTransport;
  using DownStreamChannel = StreamTransport;
  using DatagramChannel = DatagramTransport;

  using SelfT = TrustedConnectionManager;

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

    bus_.subscribe<Order>(CRefHandler<Order>::template bind<SelfT, &SelfT::post>(this));
    bus_.subscribe<ConnectionStatusEvent>(
        CRefHandler<ConnectionStatusEvent>::template bind<SelfT, &SelfT::post>(this));
  }

  void close() { reset(); }

private:
  void onUpstreamConnected(StreamTransport &&transport) {
    if (upstreamChannel_) {
      LOG_ERROR_SYSTEM("Already connected upstream");
      return;
    }
    LOG_INFO_SYSTEM("Connected upstream");
    upstreamChannel_ = std::make_unique<UpStreamChannel>(std::move(transport));
    notify();
  }

  void onDownstreamConnected(StreamTransport &&transport) {
    if (downstreamChannel_) {
      LOG_ERROR_SYSTEM("Already connected downstream");
      return;
    }
    LOG_INFO_SYSTEM("Connected downstream");
    downstreamChannel_ = std::make_unique<DownStreamChannel>(std::move(transport));

    ByteSpan span(reinterpret_cast<uint8_t *>(&status_), sizeof(OrderStatus));
    downstreamChannel_->asyncRx( // format
        span, CRefHandler<IoResult>::template bind<SelfT, &SelfT::post>(this));
    notify();
  }

  void onDatagramConnected(DatagramTransport &&transport) {
    if (pricesChannel_) {
      LOG_ERROR_SYSTEM("Already connected datagram");
      return;
    }
    LOG_INFO_SYSTEM("Connected datagram");
    pricesChannel_ = std::make_unique<DatagramChannel>(std::move(transport));
  }

  void post(CRef<IoResult> res) {
    if (res.code == IoStatus::Ok) {
      bus_.post(status_);
    } else {
      LOG_ERROR_SYSTEM("Failed to read from shm, stopping");
      reset();
    }
  }

  void post(CRef<Order> order) {
    LOG_DEBUG("{}", toString(order));
    auto *ptr = reinterpret_cast<const uint8_t *>(&order);
    CByteSpan span(ptr, sizeof(Order));
    auto res = upstreamChannel_->syncTx(span);
    if (!res) {
      LOG_ERROR_SYSTEM("Failed to write to shm, stopping");
      reset();
    }
  }

  void post(CRef<ConnectionStatusEvent> event) {
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
    if (upstreamChannel_) {
      upstreamChannel_->close();
    }
    if (downstreamChannel_) {
      downstreamChannel_->close();
    }
    if (pricesChannel_) {
      pricesChannel_->close();
    }
    bus_.post(ClientState::Disconnected);
  }

private:
  ClientBus &bus_;

  ShmClient &networkClient_;

  UPtr<UpStreamChannel> upstreamChannel_;
  UPtr<DownStreamChannel> downstreamChannel_;
  UPtr<DatagramChannel> pricesChannel_;

  OrderStatus status_;
};
} // namespace hft::client

#endif // HFT_CLIENT_CONNECTIONMANAGER_HPP
