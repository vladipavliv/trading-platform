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
  explicit MonitorConfig(const String &fileName);

  void print() const;

  // cores
  Optional<CoreId> coreSystem;
  double nsPerCycle;

  // Logging
  String logOutput;

  Config data;
};

} // namespace hft::monitor

#endif // HFT_MONITOR_CONFIG_HPP