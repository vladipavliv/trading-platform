#include "config/config.hpp"
#include "hft_trader.hpp"
#include "init_logger.hpp"

int main() {
  using namespace hft;
  try {
    initLogger();
    Config::logTraderConfig(Config::config().trader);

    trader::HftTrader trader;
    trader.start();
  } catch (const std::exception &ex) {
    spdlog::error("Exception in HftServer: {}", ex.what());
  }
  return 0;
}