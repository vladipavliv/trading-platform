/**
 * @author Vladimir Pavliv
 * @date 2025-03-06
 */

#ifndef HFT_MONITOR_SERVERCOMMAND_HPP
#define HFT_MONITOR_SERVERCOMMAND_HPP

#include <stdint.h>

#include "types.hpp"
#include "utils/string_utils.hpp"

namespace hft::monitor {
/**
 * @brief Native monitor command
 */
enum class MonitorCommand : uint8_t { Shutdown };
} // namespace hft::monitor

namespace hft::utils {
inline String toString(const monitor::MonitorCommand &command) {
  using namespace monitor;
  switch (command) {
  case MonitorCommand::Shutdown:
    return "shutdown";
  default:
    return std::format("unknown command {}", static_cast<uint8_t>(command));
  }
}
} // namespace hft::utils

#endif // HFT_SERVER_SERVERCOMMAND_HPP
