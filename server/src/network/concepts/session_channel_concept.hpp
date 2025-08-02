/**
 * @author Vladimir Pavliv
 * @date 2025-07-27
 */

#ifndef HFT_SERVER_SESSIONCHANNELCONCEPT_HPP
#define HFT_SERVER_SESSIONCHANNELCONCEPT_HPP

#include <concepts>

#include "concepts/busable.hpp"
#include "domain_types.hpp"
#include "types.hpp"

namespace hft::server {

/**
 * @brief Defines a channel for network message exchange
 */
template <typename ChannelType>
concept SessionChannelable = Busable<ChannelType> && requires(ChannelType &chan, ClientId id) {
  { chan.isAuthenticated() } -> std::same_as<bool>;
  { chan.connectionId() } -> std::same_as<ConnectionId>;
  { chan.clientId() } -> std::same_as<std::optional<ClientId>>;
  { chan.authenticate(id) } -> std::same_as<void>;
};

} // namespace hft::server

#endif // HFT_SERVER_SESSIONCHANNELCONCEPT_HPP
