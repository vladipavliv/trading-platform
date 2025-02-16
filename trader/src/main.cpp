/**
 * @file
 * @brief
 *
 * @author Vladimir Pavliv
 * @date 2025-02-13
 */

#include "config/config_reader.hpp"
#include "hft_trader.hpp"
#include "init_logger.hpp"

#include <atomic>
#include <iostream>
#include <thread>

int main(int argc, char *argv[]) {
  using namespace hft;

  // initAsyncLogger("hft_trader.txt");
  initLogger();
  ConfigReader::readConfig();

  trader::HftTrader trader;
  trader.start();
  return 0;
}