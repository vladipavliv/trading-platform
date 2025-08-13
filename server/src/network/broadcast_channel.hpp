/**
 * @author Vladimir Pavliv
 * @date 2025-03-29
 */

#ifndef HFT_SERVER_BROADCASTCHANNEL_HPP
#define HFT_SERVER_ROADCASTCHANNEL_HPP

#include "boost_types.hpp"
#include "config/server_config.hpp"
#include "network/channels/udp_channel.hpp"
#include "server_types.hpp"
#include "types.hpp"

namespace hft::server {

/**
 * @brief Broadcasts messages (TickerPrice) over the udp socket
 */
class BroadcastChannel {
public:
  BroadcastChannel(ConnectionId id, IoCtx &ioCtx, ServerBus &bus)
      : id_{id}, ioCtx_{ioCtx}, bus_{bus},
        udpChannel_{utils::generateConnectionId(), utils::createUdpSocket(ioCtx_),
                    UdpEndpoint{Ip::address_v4::broadcast(), ServerConfig::cfg.portUdp}, bus_} {
    bus_.subscribe<TickerPrice>([this](CRef<TickerPrice> p) { udpChannel_.write(p); });
  }

  inline auto connectionId() const -> ConnectionId { return id_; }

private:
  const ConnectionId id_;

  IoCtx &ioCtx_;
  ServerBus &bus_;

  UdpChannel<ServerBus> udpChannel_;
};

} // namespace hft::server

#endif // HFT_SERVER_BROADCASTCHANNEL_HPP