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
  try {
    using namespace hft;
    Logger::initialize(spdlog::level::trace, "server_log.txt");
    ConfigReader::readConfig("server_config.ini");

    Logger::monitorLogger->info("Server configuration:");
    Config::cfg.logConfig();
    Logger::monitorLogger->info("LogLevel:{}", utils::toString(spdlog::get_level()));

    server::HftServer server;
    server.start();
  } catch (const std::exception &e) {
    std::cerr << e.what() << '\n';
  }
  return 0;
}