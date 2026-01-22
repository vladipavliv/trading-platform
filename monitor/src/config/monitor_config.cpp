/**
 * @author Vladimir Pavliv
 * @date 2025-04-11
 */

#include "monitor_config.hpp"
#include "config/config.hpp"
#include "logging.hpp"
#include "primitive_types.hpp"
#include "ptr_types.hpp"
#include "utils/time_utils.hpp"

namespace hft::monitor {

MonitorConfig::MonitorConfig(CRef<String> fileName) : data{fileName} {
  // cores
  if (auto core = data.get_optional<size_t>("cpu.core_system")) {
    coreSystem = *core;
  }

  nsPerCycle = utils::getNsPerCycle();

  // Logging
  logOutput = data.get<String>("log.output");

  LOG_INFO_SYSTEM("LogOutput: {}", logOutput);
}

} // namespace hft::monitor
