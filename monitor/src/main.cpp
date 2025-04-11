/**
 * @author Vladimir Pavliv
 * @date 2025-04-11
 */

#include <iostream>

#include "config/monitor_config.hpp"
#include "config/monitor_config_reader.hpp"
#include "logging.hpp"
#include "monitor_control_center.hpp"

int main(int argc, char *argv[]) {
  using namespace hft;
  using namespace monitor;
  try {
    MonitorConfigReader::readConfig("monitor_config.ini");
    LOG_INIT(MonitorConfig::cfg.logOutput);

    MonitorControlCenter monitorCc;
    monitorCc.start();
  } catch (const std::exception &e) {
    std::cerr << "Exception caught in main {}" << e.what() << std::endl;
  }
  return 0;
}
