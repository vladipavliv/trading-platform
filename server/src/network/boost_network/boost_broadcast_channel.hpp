/**
 * @author Vladimir Pavliv
 * @date 2025-03-29
 */

#ifndef HFT_SERVER_BOOSTBROADCASTCHANNEL_HPP
#define HFT_SERVER_BOOSTBROADCASTCHANNEL_HPP

#include "boost_types.hpp"
#include "config/server_config.hpp"
#include "network/transport/udp_transport.hpp"
#include "server_types.hpp"
#include "types.hpp"

namespace hft::server {

/**
 * @brief Broadcasts messages (TickerPrice) over the udp socket
 */
class BoostBroadcastChannel {
public:
  BoostBroadcastChannel(ConnectionId id, IoCtx &ioCtx, ServerBus &bus)
      : id_{id}, ioCtx_{ioCtx}, bus_{bus},
        udpTransport_{utils::generateConnectionId(), utils::createUdpSocket(ioCtx_),
                      UdpEndpoint{Ip::address_v4::broadcast(), ServerConfig::cfg.portUdp}, bus_} {
    bus_.subscribe<TickerPrice>([this](CRef<TickerPrice> p) { udpTransport_.write(p); });
  }

  inline auto connectionId() const -> ConnectionId { return id_; }

private:
  const ConnectionId id_;

  IoCtx &ioCtx_;
  ServerBus &bus_;

  UdpTransport<ServerBus> udpTransport_;
};

} // namespace hft::server

#endif // HFT_SERVER_BOOSTBROADCASTCHANNEL_HPP