/**
 * @author Vladimir Pavliv
 * @date 2025-03-29
 */

#ifndef HFT_SERVER_TRUSTEDSESSIONMANAGER_HPP
#define HFT_SERVER_TRUSTEDSESSIONMANAGER_HPP

#include "bus/bus_hub.hpp"
#include "constants.hpp"
#include "events.hpp"
#include "logging.hpp"
#include "traits.hpp"
#include "transport/channel.hpp"
#include "transport/connection_status.hpp"
#include "transport/session_channel.hpp"
#include "utils/id_utils.hpp"
#include "utils/string_utils.hpp"

namespace hft::server {

/**
 * @brief
 */
class TrustedSessionManager {
  using UpstreamChan = StreamTransport;
  using DownstreamChan = StreamTransport;

public:
  explicit TrustedSessionManager(ServerBus &bus) : bus_{bus} {
    LOG_INFO_SYSTEM("TrustedSessionManager initialized");
    bus_.subscribe<ServerOrderStatus>(
        [this](CRef<ServerOrderStatus> event) { onOrderStatus(event); });
    bus_.subscribe<ChannelStatusEvent>(
        [this](CRef<ChannelStatusEvent> event) { onChannelStatus(event); });
  }

  void acceptUpstream(StreamTransport &&t) {
    upChannel_ = std::make_unique<UpstreamChan>(std::move(t));

    ByteSpan span(reinterpret_cast<uint8_t *>(&order_), sizeof(Order));
    upChannel_->asyncRx(span, [this](IoResult, size_t) { bus_.post(ServerOrder{0, order_}); });
  }

  void acceptDownstream(StreamTransport &&t) {
    downChannel_ = std::make_unique<DownstreamChan>(std::move(t));
  }

  void close() {
    LOG_DEBUG("TrustedSessionManager close");
    if (upChannel_) {
      upChannel_->close();
      upChannel_.reset();
    }
    if (downChannel_) {
      downChannel_->close();
      downChannel_.reset();
    }
  }

  void onChannelStatus(CRef<ChannelStatusEvent> event) {
    LOG_DEBUG("{}", toString(event));
    switch (event.event.status) {
    case ConnectionStatus::Connected:
      break;
    case ConnectionStatus::Disconnected:
    case ConnectionStatus::Error:
      LOG_DEBUG("Channel {} disconnected", event.event.connectionId);
      close();
      break;
    default:
      break;
    }
  }

private:
  void onOrderStatus(CRef<ServerOrderStatus> status) {
    LOG_DEBUG("{}", toString(status.orderStatus));
    auto *ptr = reinterpret_cast<const uint8_t *>(&status.orderStatus);
    CByteSpan span(ptr, sizeof(OrderStatus));
    downChannel_->asyncTx(span, [](IoResult, size_t) {});
  }

private:
  ServerBus &bus_;

  Order order_;

  UPtr<UpstreamChan> upChannel_;
  UPtr<DownstreamChan> downChannel_;
};

} // namespace hft::server

#endif // HFT_SERVER_TRUSTEDSESSIONMANAGER_HPP
