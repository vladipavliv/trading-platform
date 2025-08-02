/**
 * @author Vladimir Pavliv
 * @date 2025-07-27
 */

#ifndef HFT_SERVER_BROADCASTCHANNELCONCEPT_HPP
#define HFT_SERVER_BROADCASTCHANNELCONCEPT_HPP

#include <concepts>

#include "concepts/busable.hpp"
#include "types.hpp"

namespace hft::server {

/**
 * @brief Defines a channel for broadcast network message exchange
 * For now for convenience works with busable interface bi-directionally,
 * accepts incoming and outgoing messages via bus interface
 */
template <typename ChannelType>
concept BroadcastChannelable = requires(ChannelType channel) {
  { channel.connectionId() } -> std::same_as<ConnectionId>;
};

} // namespace hft::server

#endif // HFT_SERVER_BROADCASTCHANNELCONCEPT_HPP
