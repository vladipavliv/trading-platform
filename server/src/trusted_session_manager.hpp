/**
 * @author Vladimir Pavliv
 * @date 2025-03-29
 */

#ifndef HFT_SERVER_TRUSTEDSESSIONMANAGER_HPP
#define HFT_SERVER_TRUSTEDSESSIONMANAGER_HPP

#include "constants.hpp"
#include "events.hpp"
#include "logging.hpp"
#include "network/channel.hpp"
#include "network/connection_status.hpp"
#include "network/session_channel.hpp"
#include "traits.hpp"
#include "utils/id_utils.hpp"
#include "utils/string_utils.hpp"

namespace hft::server {

/**
 * @brief
 */
class TrustedSessionManager {
  using TrustedUpstreamBus = BusRestrictor< // format
      ServerBus, ServerOrder, ChannelStatusEvent, ConnectionStatusEvent>;
  using TrustedDownstreamBus = BusRestrictor< // format
      ServerBus, ChannelStatusEvent, ConnectionStatusEvent>;

  using UpstreamChan = SessionChannel<TrustedUpstreamBus>;
  using DownstreamChan = SessionChannel<TrustedDownstreamBus>;

public:
  explicit TrustedSessionManager(ServerBus &bus) : bus_{bus} {
    LOG_INFO_SYSTEM("TrustedSessionManager initialized");
    bus_.subscribe<ServerOrderStatus>(
        [this](CRef<ServerOrderStatus> event) { onOrderStatus(event); });
    bus_.subscribe<ChannelStatusEvent>(
        [this](CRef<ChannelStatusEvent> event) { onChannelStatus(event); });
  }

  void acceptUpstream(StreamTransport &&t) {
    const auto id = utils::generateConnectionId();
    LOG_INFO_SYSTEM("New upstream connection id: {}", id);
    upChannel_ = std::make_shared<UpstreamChan>(std::move(t), id, TrustedUpstreamBus{bus_});
    upChannel_->authenticate(0);
    upChannel_->read();
  }

  void acceptDownstream(StreamTransport &&t) {
    const auto id = utils::generateConnectionId();
    LOG_INFO_SYSTEM("New downstream connection Id: {}", id);
    downChannel_ = std::make_shared<DownstreamChan>(std::move(t), id, TrustedDownstreamBus{bus_});
    downChannel_->authenticate(0);
    downChannel_->read();
  }

  void close() {
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
    LOG_DEBUG("{}", toString(status));
    auto chan = downChannel_;
    if (chan) {
      chan->write(status.orderStatus);
    }
  }

private:
  ServerBus &bus_;

  SPtr<UpstreamChan> upChannel_;
  SPtr<DownstreamChan> downChannel_;
};

} // namespace hft::server

#endif // HFT_SERVER_TRUSTEDSESSIONMANAGER_HPP
