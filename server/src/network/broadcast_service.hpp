/**
 * @author Vladimir Pavliv
 * @date 2025-03-29
 */

#ifndef HFT_SERVER_BROADCAST_SERVICE_HPP
#define HFT_SERVER_BROADCAST_SERVICE_HPP

#include "boost_types.hpp"
#include "config/server_config.hpp"
#include "network/transport/udp_transport.hpp"
#include "network_types.hpp"
#include "types.hpp"

namespace hft::server {

/**
 * @brief Broadcasts messages (TickerPrice) over the udp socket
 */
class BroadcastService {
public:
  BroadcastService(IoCtx &ioCtx, Bus &bus)
      : ioCtx_{ioCtx}, bus_{bus},
        udpTransport_{utils::createUdpSocket(ioCtx_),
                      UdpEndpoint{Ip::address_v4::broadcast(), ServerConfig::cfg.portUdp}, bus_} {
    bus_.marketBus.setHandler<TickerPrice>([this](CRef<TickerPrice> p) { udpTransport_.write(p); });
  }

private:
  IoCtx &ioCtx_;
  Bus &bus_;

  UdpTransport<> udpTransport_;
};

} // namespace hft::server

#endif // HFT_SERVER_BROADCAST_SERVICE_HPP