/**
 * @author Vladimir Pavliv
 * @date 2025-03-06
 */

#ifndef HFT_SERVER_SERVEREVENTS_HPP
#define HFT_SERVER_SERVEREVENTS_HPP

#include "types.hpp"
#include "utils/string_utils.hpp"

namespace hft {
namespace server {
/**
 * @brief
 */
enum class ServerEvent : uint8_t { Ready };
} // namespace server

namespace utils {
template <>
std::string toString<server::ServerEvent>(const server::ServerEvent &event) {
  using namespace server;
  switch (event) {
  case ServerEvent::Ready:
    return "ready";
  default:
    return std::format("unknown server event {}", static_cast<uint8_t>(event));
  }
}
} // namespace utils
} // namespace hft

#endif // HFT_SERVER_SERVEREVENTS_HPP