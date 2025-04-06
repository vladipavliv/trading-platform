/**
 * @author Vladimir Pavliv
 * @date 2025-03-06
 */

#ifndef HFT_SERVER_TRADEREVENTS_HPP
#define HFT_SERVER_TRADEREVENTS_HPP

#include <stdint.h>

#include "utils/string_utils.hpp"

namespace hft {
namespace trader {
/**
 * @brief System events
 */
enum class TraderEvent : uint8_t { ConnectedToTheServer, DisconnectedFromTheServer };
} // namespace trader

namespace utils {
template <>
std::string toString<trader::TraderEvent>(const trader::TraderEvent &event) {
  using namespace trader;
  switch (event) {
  case TraderEvent::ConnectedToTheServer:
    return "connected to the server";
  case TraderEvent::DisconnectedFromTheServer:
    return "disconnected from the server";
  default:
    return std::format("unknown event {}", static_cast<uint8_t>(event));
  }
}
} // namespace utils
} // namespace hft

#endif // HFT_SERVER_TRADEREVENTS_HPP