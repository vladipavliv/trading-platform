/**
 * @author Vladimir Pavliv
 * @date 2025-02-13
 */

#include <iostream>

#include "config/config.hpp"
#include "config/config_reader.hpp"
#include "logger.hpp"
#include "server.hpp"
#include "server_command.hpp"
#include "utils/string_utils.hpp"

int main() {
  using namespace hft;
  std::unique_ptr<server::Server> hftServer;
  try {
    ConfigReader::readConfig("server_config.ini");
    Logger::initialize(Config::cfg.logLevel, Config::cfg.logOutput);

    Logger::monitorLogger->info("Server go stonks {}", std::string(38, '~'));
    Logger::monitorLogger->info("Configuration:");
    Config::cfg.logConfig();

    hftServer = std::make_unique<server::Server>();
    Logger::monitorLogger->info(std::string(55, '~'));

    hftServer->start();
    Logger::monitorLogger->info("stonk");
  } catch (const std::exception &e) {
    std::cerr << "Exception caught in main " << e.what();
    spdlog::critical("Exception caught in main {}", e.what());
    hftServer->stop();
  }
  return 0;
}