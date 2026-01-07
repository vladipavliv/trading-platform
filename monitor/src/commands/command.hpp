/**
 * @author Vladimir Pavliv
 * @date 2025-03-06
 */

#ifndef HFT_MONITOR_COMMAND_HPP
#define HFT_MONITOR_COMMAND_HPP

#include <stdint.h>

#include "primitive_types.hpp"
#include "utils/string_utils.hpp"

namespace hft {
namespace monitor {
/**
 * @brief Native monitor command
 */
enum class Command : uint8_t { Shutdown };
} // namespace monitor
inline String toString(const monitor::Command &command) {
  using namespace monitor;
  switch (command) {
  case monitor::Command::Shutdown:
    return "shutdown";
  default:
    return std::format("unknown command {}", static_cast<uint8_t>(command));
  }
}
} // namespace hft

#endif // HFT_SERVER_COMMAND_HPP
