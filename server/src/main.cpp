/**
 * @author Vladimir Pavliv
 * @date 2025-02-13
 */

#include "config/config.hpp"
#include "config/config_reader.hpp"
#include "hft_server.hpp"
#include "logger.hpp"
#include "template_types.hpp"
#include "utils/string_utils.hpp"

int main() {
  using namespace hft;
  std::unique_ptr<server::HftServer> server;
  try {
    Logger::initialize(spdlog::level::trace, "server_log.txt");
    ConfigReader::readConfig("server_config.ini");

    Logger::monitorLogger->info("Server configuration:");
    Config::cfg.logConfig();
    Logger::monitorLogger->info("LogLevel:{}", utils::toString(spdlog::get_level()));

    server = std::make_unique<server::HftServer>();
    server->start();
  } catch (const std::exception &e) {
    Logger::monitorLogger->critical("Exception caught in main {}", e.what());
    spdlog::critical("Exception caught in main {}", e.what());
    spdlog::default_logger()->flush();
    server->stop();
  }
  return 0;
}