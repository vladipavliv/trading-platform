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

void MonitorConfig::load(CRef<String> fileName) {
  Config::load(fileName);

  // cores
  if (auto core = Config::get_optional<size_t>("cpu.core_system")) {
    MonitorConfig::cfg().coreSystem = *core;
  }

  nsPerCycle = utils::getNsPerCycle();

  // Logging
  MonitorConfig::cfg().logOutput = Config::get<String>("log.output");
}

void MonitorConfig::log() { LOG_INFO_SYSTEM("LogOutput: {}", logOutput); }

} // namespace hft::monitor
