/**
 * @author Vladimir Pavliv
 * @date 2025-03-06
 */

#ifndef HFT_SERVER_SERVEREVENTS_HPP
#define HFT_SERVER_SERVEREVENTS_HPP

#include "network/connection_status.hpp"
#include "types.hpp"
#include "utils/string_utils.hpp"

namespace hft {
namespace server {
/**
 * @brief Server event
 */
enum class ServerEvent : uint8_t { Ready };

struct ChannelStatusEvent {
  Opt<ClientId> clientId;
  ConnectionStatusEvent event;
};
} // namespace server

namespace utils {
String toString(const server::ServerEvent &event) {
  using namespace server;
  switch (event) {
  case ServerEvent::Ready:
    return "ready";
  default:
    return std::format("unknown server event {}", static_cast<uint8_t>(event));
  }
}
String toString(const server::ChannelStatusEvent &event) {
  using namespace server;
  return std::format("ChannelStatusEvent {} {}", event.clientId.value_or(0),
                     utils::toString(event.event));
}
} // namespace utils
} // namespace hft

#endif // HFT_SERVER_SERVEREVENTS_HPP