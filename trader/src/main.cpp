/**
 * @author Vladimir Pavliv
 * @date 2025-02-13
 */

#include "config/config.hpp"
#include "config/config_reader.hpp"
#include "event_fd.hpp"
#include "hft_trader.hpp"
#include "logger_manager.hpp"
#include "utils/string_utils.hpp"

int main(int argc, char *argv[]) {
  try {
    using namespace hft;
    LoggerManager::initialize(LoggerManager::LoggerMode::Console, spdlog::level::debug);
    ConfigReader::readConfig("trader_config.ini");

    spdlog::info("Trader configuration:");
    Config::cfg.logConfig();
    spdlog::info("LogLevel:{}", utils::toString(spdlog::get_level()));

    trader::HftTrader trader;
    trader.start();
  } catch (const std::exception &e) {
    std::cerr << e.what() << '\n';
  }
  return 0;
}