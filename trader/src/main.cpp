/**
 * @author Vladimir Pavliv
 * @date 2025-02-13
 */

#include <iostream>

#include "config/config.hpp"
#include "config/config_reader.hpp"
#include "logger.hpp"
#include "trader_command.hpp"
#include "trader_control_center.hpp"
#include "utils/string_utils.hpp"

int main(int argc, char *argv[]) {
  using namespace hft;
  using namespace trader;
  try {
    ConfigReader::readConfig("trader_config.ini");
    Logger::initialize(Config::cfg.logLevel, Config::cfg.logOutput);

    TraderControlCenter traderCc;
    traderCc.start();
  } catch (const std::exception &e) {
    Logger::monitorLogger->critical("Exception caught in main {}", e.what());
    spdlog::critical("Exception caught in main {}", e.what());
  }
  return 0;
}
