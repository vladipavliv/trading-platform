/**
 * @author Vladimir Pavliv
 * @date 2025-03-06
 */

#ifndef HFT_SERVER_SERVEREVENTS_HPP
#define HFT_SERVER_SERVEREVENTS_HPP

#include "functional_types.hpp"
#include "primitive_types.hpp"
#include "status_code.hpp"
#include "transport/connection_status.hpp"
#include "utils/string_utils.hpp"

namespace hft {
namespace server {
/**
 * @brief Server event
 */
enum class ServerState : uint8_t { Operational, InternalError };

struct ServerEvent {
  ServerState state;
  StatusCode code;
};

// TODO
struct ChannelStatusEvent {
  Optional<ClientId> clientId;
  ConnectionStatusEvent event;
};
} // namespace server

inline String toString(const server::ServerState &event) {
  using namespace server;
  switch (event) {
  case server::ServerState::Operational:
    return "server is fully operational";
  case server::ServerState::InternalError:
    return "internal error";
  default:
    return std::format("unknown server event {}", static_cast<uint8_t>(event));
  }
}
inline String toString(const server::ServerEvent &event) {
  using namespace server;
  return std::format("ServerEvent {} {}", toString(event.state), toString(event.code));
}
inline String toString(const server::ChannelStatusEvent &event) {
  using namespace server;
  return std::format("ChannelStatusEvent {} {}", event.clientId.value_or(0), toString(event.event));
}
} // namespace hft

#endif // HFT_SERVER_SERVEREVENTS_HPP