/**
 * @author Vladimir Pavliv
 * @date 2025-02-13
 */

#include <iostream>

#include "config/config_reader.hpp"
#include "hft_trader.hpp"
#include "logger_manager.hpp"
#include "utils/string_utils.hpp"

int main(int argc, char *argv[]) {
  try {
    using namespace hft;
    LoggerManager::initConsoleLogger(spdlog::level::debug);
    ConfigReader::readConfig();

    spdlog::info("Trader configuration:");
    String cfg = Config::cfg.toString();
    spdlog::info("{}", cfg);
    size_t tradeRate = TRADE_RATE; // In microseconds
    spdlog::info("Trading rate:{}μs LogLevel:{}", tradeRate, utils::toString(spdlog::get_level()));

    trader::HftTrader trader;
    trader.start();
  } catch (const std::exception &e) {
    std::cerr << e.what() << '\n';
  }
  return 0;
}