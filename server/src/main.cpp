/**
 * @author Vladimir Pavliv
 * @date 2025-02-13
 */

#include <iostream>

#include "config/config.hpp"
#include "config/config_reader.hpp"
#include "logger.hpp"
#include "server_command.hpp"
#include "server_control_center.hpp"
#include "utils/string_utils.hpp"

int main(int argc, char *argv[]) {
  using namespace hft;
  using namespace server;
  try {
    ConfigReader::readConfig("server_config.ini");
    Logger::initialize(Config::cfg.logLevel, Config::cfg.logOutput);

    ServerControlCenter serverCc;
    serverCc.start();
  } catch (const std::exception &e) {
    Logger::monitorLogger->critical("Exception caught in main {}", e.what());
    spdlog::critical("Exception caught in main {}", e.what());
  }
  return 0;
}