/**
 * @author Vladimir Pavliv
 * @date 2025-02-13
 */

#include "config/config.hpp"
#include "config/config_reader.hpp"
#include "logger.hpp"
#include "trader.hpp"
#include "trader_utils.hpp"
#include "utils/string_utils.hpp"

int main(int argc, char *argv[]) {
  using namespace hft;
  std::unique_ptr<trader::Trader> trader;
  try {
    ConfigReader::readConfig("trader_config.ini");
    Logger::initialize(Config::cfg.logLevel, Config::cfg.logOutput);

    Logger::monitorLogger->info("Trader configuration:");
    Config::cfg.logConfig();

    trader = std::make_unique<trader::Trader>();
    trader->start();
  } catch (const std::exception &e) {
    std::cerr << "Exception caught in main" << e.what();
    spdlog::critical("Exception caught in main {}", e.what());
    trader->stop();
  }
  return 0;
}