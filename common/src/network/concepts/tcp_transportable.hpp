/**
 * @author Vladimir Pavliv
 * @date 2025-07-27
 */

#ifndef HFT_COMMON_TCPTRANSPORTABLE_HPP
#define HFT_COMMON_TCPTRANSPORTABLE_HPP

#include <concepts>

#include "concepts/busable.hpp"
#include "domain_types.hpp"
#include "network/connection_status.hpp"

namespace hft {

template <typename TcpTransportType>
concept TcpTransportable =
    Busable<TcpTransportType> && requires(TcpTransportType &chan, ClientId id) {
      { chan.connectionId() } -> std::same_as<ConnectionId>;
      { chan.connect() } -> std::same_as<void>;
      { chan.read() } -> std::same_as<void>;
      { chan.reconnect() } -> std::same_as<void>;
      { chan.isConnected() } -> std::same_as<bool>;
      { chan.status() } -> std::same_as<ConnectionStatus>;
      { chan.close() } -> std::same_as<void>;
    };

} // namespace hft

#endif // HFT_COMMON_TCPTRANSPORTABLE_HPP
