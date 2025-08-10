/**
 * @author Vladimir Pavliv
 * @date 2025-07-27
 */

#ifndef HFT_COMMON_NETWORKLAYERABLE_HPP
#define HFT_COMMON_NETWORKLAYERABLE_HPP

#include <concepts>

#include "network_listenerable.hpp"
#include "tcp_transportable.hpp"
#include "udp_transportable.hpp"

namespace hft {

/**
 * @brief Defines a network layer templated by the bus type and listener type
 * @details Network layer and listener are co-dependent: listener is accepting concrete transport
 * types defined by the network layer, but network layer should be able to accept concrete listeners
 * So network layer is required to expose tcp and udp transport types, that listener is required to
 * accept, and network layer is required to accept listener.
 * Connections are not routed via the bus, as its a message router with no bond between
 * producer and consumer. New connections should be handled explicitly and strictly.
 */
template <template <typename, typename> typename NetworkLayer, typename BusType, typename Listener>
concept NetworkLayerable =
    Busable<BusType> &&
    TcpTransportable<typename NetworkLayer<BusType, Listener>::TcpTransportType> &&
    UdpTransportable<typename NetworkLayer<BusType, Listener>::UdpTransportType> &&
    NetworkListenerable<Listener, typename NetworkLayer<BusType, Listener>::TcpTransportType> &&
    requires(NetworkLayer<BusType, Listener> &network, BusType &bus, Listener &listener,
             CRef<String> url, Port port, bool broadcast) {
      { NetworkLayer<BusType, Listener>{bus, listener} };
      {
        network.createTcp(url, port)
      } -> std::same_as<typename NetworkLayer<BusType, Listener>::TcpTransportType>;
      {
        network.createUdp(port, broadcast)
      } -> std::same_as<typename NetworkLayer<BusType, Listener>::UdpTransportType>;
      { network.listen(port) } -> std::same_as<void>;
      { network.start() } -> std::same_as<void>;
      { network.stop() } -> std::same_as<void>;
    };

} // namespace hft

#endif // HFT_COMMON_NETWORKLAYERABLE_HPP
