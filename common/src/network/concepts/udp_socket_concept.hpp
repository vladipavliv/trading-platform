/**
 * @author Vladimir Pavliv
 * @date 2025-07-27
 */

#ifndef HFT_COMMON_UDPTRANSPORTABLE_HPP
#define HFT_COMMON_UDPTRANSPORTABLE_HPP

#include <concepts>

#include "concepts/bus_concept.hpp"
#include "domain_types.hpp"

namespace hft {

template <typename UdpSocket>
concept UdpSocketConcept = [] {
  struct ProbeType {};
  return requires(UdpSocket &sock) {
    { sock.template write<ProbeType>(std::declval<ProbeType>()) } -> std::same_as<void>;
    { sock.connectionId() } -> std::same_as<ConnectionId>;
    { sock.connect() } -> std::same_as<void>;
    { sock.reconnect() } -> std::same_as<void>;
    { sock.read() } -> std::same_as<void>;
    { sock.close() } -> std::same_as<void>;
  };
}();

} // namespace hft

#endif // HFT_COMMON_UDPTRANSPORTABLE_HPP
