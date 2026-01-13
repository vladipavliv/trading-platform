/**
 * @author Vladimir Pavliv
 * @date 2025-04-11
 */

#ifndef HFT_MONITOR_CONFIG_HPP
#define HFT_MONITOR_CONFIG_HPP

#include "config/config.hpp"
#include "functional_types.hpp"
#include "primitive_types.hpp"
#include "ptr_types.hpp"

namespace hft::monitor {

struct MonitorConfig {
  static MonitorConfig cfg;

  // cores
  Optional<CoreId> coreSystem;
  double nsPerCycle;

  // Logging
  String logOutput;

  static void load(CRef<String> fileName);
  static void log();
};

} // namespace hft::monitor

#endif // HFT_MONITOR_CONFIG_HPP