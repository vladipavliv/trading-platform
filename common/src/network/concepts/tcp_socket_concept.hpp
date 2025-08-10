/**
 * @author Vladimir Pavliv
 * @date 2025-07-27
 */

#ifndef HFT_COMMON_TCPTRANSPORTABLE_HPP
#define HFT_COMMON_TCPTRANSPORTABLE_HPP

#include <concepts>

#include "concepts/bus_concept.hpp"
#include "domain_types.hpp"
#include "network/connection_status.hpp"

namespace hft {

template <typename TcpSocket>
concept TcpSocketConcept = [] {
  struct ProbeType {};
  return requires(TcpSocket &sock) {
    { sock.template write<ProbeType>(std::declval<ProbeType>()) } -> std::same_as<void>;
    { sock.connectionId() } -> std::same_as<ConnectionId>;
    { sock.connect() } -> std::same_as<void>;
    { sock.reconnect() } -> std::same_as<void>;
    { sock.isConnected() } -> std::same_as<bool>;
    { sock.read() } -> std::same_as<void>;
    { sock.status() } -> std::same_as<ConnectionStatus>;
    { sock.close() } -> std::same_as<void>;
  };
}();

} // namespace hft

#endif // HFT_COMMON_TCPTRANSPORTABLE_HPP
