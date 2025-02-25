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
    Logger::initialize(spdlog::level::err, "server_log.txt");
    ConfigReader::readConfig("server_config.ini");

    Logger::monitorLogger->info("Server configuration:");
    Config::cfg.logConfig();
    Logger::monitorLogger->info("LogLevel:{}", utils::toString(spdlog::get_level()));

    hftServer = std::make_unique<server::Server>();
    hftServer->start();
  } catch (const std::exception &e) {
    Logger::monitorLogger->critical("Exception caught in main {}", e.what());
    spdlog::critical("Exception caught in main {}", e.what());
    spdlog::default_logger()->flush();
    hftServer->stop();
  }
  return 0;
}