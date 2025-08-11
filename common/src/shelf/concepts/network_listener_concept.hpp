/**
 * @author Vladimir Pavliv
 * @date 2025-08-02
 */

#include <concepts>

#include "tcp_socket_concept.hpp"
#include "types.hpp"
#include "udp_socket_concept.hpp"

#ifndef HFT_COMMON_NETWORKLISTENER_HPP
#define HFT_COMMON_NETWORKLISTENER_HPP

namespace hft {

template <typename Listener, typename TcpSocket>
concept NetworkListenerConcept =
    TcpSocketConcept<TcpSocket> && requires(Listener &listener, TcpSocket &&sock) {
      { listener.acceptTcp(sock) } -> std::same_as<void>;
    };

} // namespace hft

#endif // HFT_COMMON_NETWORKLISTENERABLE_HPP