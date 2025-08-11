/**
 * @author Vladimir Pavliv
 * @date 2025-07-27
 */

#ifndef HFT_COMMON_NETWORKLAYER_HPP
#define HFT_COMMON_NETWORKLAYER_HPP

#include <concepts>

#include "network_listener_concept.hpp"
#include "tcp_socket_concept.hpp"
#include "udp_socket_concept.hpp"

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
template <template <typename> typename NetworkLayer, typename Listener>
concept NetworkLayer = // format
    TcpSocketConcept<typename NetworkLayer<Listener>::TcpSocketType> &&
    UdpSocketConcept<typename NetworkLayer<Listener>::UdpSocketType> &&
    NetworkListenerConcept<Listener, typename NetworkLayer<Listener>::TcpSocketType> &&
    requires(NetworkLayer<Listener> &network, Listener &listener) {
      { NetworkLayer<Listener>{listener} };
      {
        network.createTcp(std::declval<CRef<String>>(), std::declval<Port>())
      } -> std::same_as<typename NetworkLayer<Listener>::TcpTransportType>;
      {
        network.createUdp(std::declval<Port>(), std::declval<bool>())
      } -> std::same_as<typename NetworkLayer<Listener>::UdpTransportType>;
      { network.listen(std::declval<Port>()) } -> std::same_as<void>;
      { network.start() } -> std::same_as<void>;
      { network.stop() } -> std::same_as<void>;
    };

} // namespace hft

#endif // HFT_COMMON_NETWORKLAYERABLE_HPP
