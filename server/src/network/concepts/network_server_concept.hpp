/**
 * @author Vladimir Pavliv
 * @date 2025-07-27
 */

#ifndef HFT_SERVER_NETWORKSERVERCONCEPT_HPP
#define HFT_SERVER_NETWORKSERVERCONCEPT_HPP

#include <concepts>

#include "broadcast_channel_concept.hpp"
#include "network_listener_concept.hpp"
#include "server_types.hpp"
#include "session_channel_concept.hpp"

namespace hft::server {

/**
 * @brief Concept for a network server templated by the listener that accepts
 * ServerType::ChannelType, and has ctor that accpets Bustype and ListenerType references
 */
template <typename ServerType, typename Bustype, typename ListenerType>
concept NetworkServerable =
    Busable<Bustype> && SessionChannelable<typename ServerType::SessionChannelType> &&
    BroadcastChannelable<typename ServerType::BroadcastChannelType> &&
    NetworkListenerable<ListenerType, typename ServerType::SessionChannelType,
                        typename ServerType::BroadcastChannelType> &&
    requires(ServerType &server, Bustype &bus, ListenerType &listener) {
      { ServerType{bus, listener} };
      { server.start() } -> std::same_as<void>;
      { server.stop() } -> std::same_as<void>;
    };

} // namespace hft::server

#endif // HFT_SERVER_NETWORKSERVERCONCEPT_HPP
