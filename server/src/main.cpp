/**
 * @author Vladimir Pavliv
 * @date 2025-02-13
 */

#include "config/config.hpp"
#include "config/config_reader.hpp"
#include "logger.hpp"
#include "server.hpp"
#include "utils/string_utils.hpp"

int main() {
  using namespace hft;
  std::unique_ptr<server::Server> hftServer;
  try {
    ConfigReader::readConfig("server_config.ini");
    Logger::initialize(Config::cfg.logLevel, Config::cfg.logOutput);

    Logger::monitorLogger->info("Server configuration:");
    Config::cfg.logConfig();
    Logger::monitorLogger->info("Commands:");
    Logger::monitorLogger->info("> {:7} => start/stop price feed", "p+/p-");
    Logger::monitorLogger->info("> {:7} => quit", "q");

    hftServer = std::make_unique<server::Server>();
    hftServer->start();
  } catch (const std::exception &e) {
    Logger::monitorLogger->critical("Exception caught in main {}", e.what());
    spdlog::critical("Exception caught in main {}", e.what());
    hftServer->stop();
  }
  return 0;
}