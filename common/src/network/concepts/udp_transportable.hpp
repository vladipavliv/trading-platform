/**
 * @author Vladimir Pavliv
 * @date 2025-07-27
 */

#ifndef HFT_COMMON_UDPTRANSPORTABLE_HPP
#define HFT_COMMON_UDPTRANSPORTABLE_HPP

#include <concepts>

#include "concepts/busable.hpp"
#include "domain_types.hpp"

namespace hft {

template <typename UdpTransportType>
concept UdpTransportable =
    Busable<UdpTransportType> && requires(UdpTransportType &chan, ClientId id) {
      { chan.connectionId() } -> std::same_as<ConnectionId>;
      { chan.read() } -> std::same_as<void>;
      { chan.close() } -> std::same_as<void>;
    };

} // namespace hft

#endif // HFT_COMMON_UDPTRANSPORTABLE_HPP
