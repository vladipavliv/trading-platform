/**
 * @author Vladimir Pavliv
 * @date 2025-02-13
 */

#include <iostream>
#include <thread>

#include "config/config.hpp"
#include "config/config_reader.hpp"
#include "hft_server.hpp"
#include "logger_manager.hpp"
#include "utils/string_utils.hpp"

int main() {
  try {
    using namespace hft;
    LoggerManager::initConsoleLogger(spdlog::level::debug);
    ConfigReader::readConfig();

    spdlog::info("Server configuration:");
    String cfg = Config::cfg.toString();
    spdlog::info(cfg);
    size_t feedRate = FEED_RATE; // In microseconds
    spdlog::info("Price feed rate:{}μs LogLevel:{}", feedRate,
                 utils::toString(spdlog::get_level()));

    server::HftServer server;
    server.start();
  } catch (const std::exception &e) {
    std::cerr << e.what() << '\n';
  }
  return 0;
}