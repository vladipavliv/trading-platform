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
#include "ipc/shm/shm_client.hpp"
#include "primitive_types.hpp"
#include "traits.hpp"
#include "transport/channel.hpp"
#include "transport/connection_status.hpp"
#include "utils/handler.hpp"
#include "utils/string_utils.hpp"

namespace hft::client {

/**
 * @brief Connects to the server via shm bypassing auth procedures
 */
class TrustedConnectionManager {
  using SelfT = TrustedConnectionManager;

  using StreamTHandler = MoveHandler<StreamTransport>;
  using DatagramTHandler = MoveHandler<StreamTransport>;

  using UpStreamChannel = StreamTransport;
  using DownStreamChannel = StreamTransport;
  using DatagramChannel = DatagramTransport;

public:
  TrustedConnectionManager(Context &ctx, ShmClient &networkClient)
      : ctx_{ctx}, networkClient_{networkClient} {
    LOG_INFO_SYSTEM("TrustedConnectionManager initialized");
    networkClient_.setUpstreamClb(StreamTHandler::bind<SelfT, &SelfT::onUpstream>(this));
    networkClient_.setDownstreamClb(StreamTHandler::bind<SelfT, &SelfT::onDownstream>(this));
    networkClient_.setDatagramClb(StreamTHandler::bind<SelfT, &SelfT::onDatagram>(this));

    ctx_.bus.subscribe(CRefHandler<Order>::bind<SelfT, &SelfT::post>(this));
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
    upstreamChannel_ = std::make_unique<UpStreamChannel>(std::move(transport));
    notify();
  }

  void onDownstream(StreamTransport &&transport) {
    if (downstreamChannel_) {
      LOG_ERROR_SYSTEM("Already connected downstream");
      return;
    }
    LOG_INFO_SYSTEM("Connected downstream");
    downstreamChannel_ = std::make_unique<DownStreamChannel>(std::move(transport));

    ByteSpan span(reinterpret_cast<uint8_t *>(&status_), sizeof(OrderStatus));
    downstreamChannel_->asyncRx(span, CRefHandler<IoResult>::bind<SelfT, &SelfT::post>(this));
    notify();
  }

  void onDatagram(DatagramTransport &&transport) {
    if (pricesChannel_) {
      LOG_ERROR_SYSTEM("Already connected datagram");
      return;
    }
    LOG_INFO_SYSTEM("Connected datagram");
    pricesChannel_ = std::make_unique<DatagramChannel>(std::move(transport));
  }

  void post(CRef<IoResult> res) {
    if (res.code == IoStatus::Ok) {
      ctx_.bus.post(status_);
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
      ctx_.bus.post(ClientState::Connected);
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
    ctx_.bus.post(ClientState::Disconnected);
  }

private:
  Context &ctx_;

  ShmClient &networkClient_;

  UPtr<UpStreamChannel> upstreamChannel_;
  UPtr<DownStreamChannel> downstreamChannel_;
  UPtr<DatagramChannel> pricesChannel_;

  OrderStatus status_;
};
} // namespace hft::client

#endif // HFT_CLIENT_CONNECTIONMANAGER_HPP
