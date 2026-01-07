/**
 * @author Vladimir Pavliv
 * @date 2025-04-11
 */

#include "monitor_config.hpp"
#include "config/config.hpp"
#include "logging.hpp"
#include "primitive_types.hpp"
#include "ptr_types.hpp"

namespace hft::monitor {

MonitorConfig MonitorConfig::cfg;

void MonitorConfig::load(CRef<String> fileName) {
  Config::load(fileName);

  // cores
  if (auto core = Config::get_optional<size_t>("cpu.core_system")) {
    MonitorConfig::cfg.coreSystem = *core;
  }

  // Logging
  MonitorConfig::cfg.logOutput = Config::get<String>("log.output");
}

void MonitorConfig::log() { LOG_INFO_SYSTEM("LogOutput: {}", cfg.logOutput); }

} // namespace hft::monitor
