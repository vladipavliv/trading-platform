#include "hft_trader.hpp"
#include "init_logger.hpp"

int main() {
  try {
    hft::initLogger();
    hft::trader::HftTrader trader;
    trader.start();
  } catch (const std::exception &ex) {
    spdlog::error("Exception in HftServer: {}", ex.what());
  }
  return 0;
}