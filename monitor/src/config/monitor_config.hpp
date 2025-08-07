/**
 * @author Vladimir Pavliv
 * @date 2025-04-11
 */

#ifndef HFT_MONITOR_CONFIG_HPP
#define HFT_MONITOR_CONFIG_HPP

#include "boost_types.hpp"
#include "config/config.hpp"
#include "logging.hpp"
#include "types.hpp"
#include "utils/string_utils.hpp"
#include "utils/utils.hpp"

namespace hft::monitor {

struct MonitorConfig {
  // cores
  Optional<CoreId> coreSystem;

  // Logging
  String logOutput;

  static MonitorConfig cfg;

  static void load(CRef<String> fileName) {
    Config::load(fileName);

    // cores
    if (auto core = Config::get_optional<size_t>("cpu.core_system")) {
      MonitorConfig::cfg.coreSystem = *core;
    }

    // Logging
    MonitorConfig::cfg.logOutput = Config::get<String>("log.output");

    verify();
  }

  static void verify() {}

  static void log() { LOG_INFO_SYSTEM("LogOutput: {}", cfg.logOutput); }
};

inline MonitorConfig MonitorConfig::cfg;

} // namespace hft::monitor

#endif // HFT_MONITOR_CONFIG_HPP