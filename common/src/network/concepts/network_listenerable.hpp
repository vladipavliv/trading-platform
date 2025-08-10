/**
 * @author Vladimir Pavliv
 * @date 2025-08-02
 */

#include <concepts>

#include "tcp_transportable.hpp"
#include "types.hpp"
#include "udp_transportable.hpp"

#ifndef HFT_COMMON_NETWORKLISTENERABLE_HPP
#define HFT_COMMON_NETWORKLISTENERABLE_HPP

namespace hft {

/**
 * @brief Defines a listener concept, that shall accept incoming network connections
 */
template <typename ListenerType, typename TcpTransportType>
concept NetworkListenerable = // format
    TcpTransportable<TcpTransportType> &&
    requires(ListenerType &listener, TcpTransportType &&tcpTransport) {
      { listener.acceptTcp(tcpTransport) } -> std::same_as<void>;
    };

} // namespace hft

#endif // HFT_COMMON_NETWORKLISTENERABLE_HPP