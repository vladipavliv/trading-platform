/**
 * @author Vladimir Pavliv
 * @date 2025-08-02
 */

#include <concepts>

#include "broadcast_channel_concept.hpp"
#include "session_channel_concept.hpp"

#ifndef HFT_SERVER_NETWORKLISTENER_HPP
#define HFT_SERVER_NETWORKLISTENER_HPP

namespace hft::server {

/**
 * @brief Defines a NetworkListsner concept that accepts channels
 * The concrete type of the channel is defined in concrete NetworkServer implementation
 */
template <typename ListenerType, typename SessionChannelType, typename BroadcastChannelType>
concept NetworkListenerable =
    SessionChannelable<SessionChannelType> && BroadcastChannelable<BroadcastChannelType> &&
    requires(ListenerType &listener, SPtr<SessionChannelType> sessionChannel,
             SPtr<BroadcastChannelType> broadcastChannel) {
      { listener.acceptUpstream(sessionChannel) } -> std::same_as<void>;
      { listener.acceptDownstream(sessionChannel) } -> std::same_as<void>;
      { listener.acceptBroadcast(broadcastChannel) } -> std::same_as<void>;
    };

} // namespace hft::server

#endif // HFT_SERVER_NETWORKLISTENER_HPP