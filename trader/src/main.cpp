/**
 * @author Vladimir Pavliv
 * @date 2025-02-13
 */

#include <atomic>
#include <iostream>
#include <thread>

#include "config/config_reader.hpp"
#include "hft_trader.hpp"
#include "init_logger.hpp"
#include "utils/string_utils.hpp"

int main(int argc, char *argv[]) {
  try {
    using namespace hft;

    // initAsyncLogger("hft_trader.txt");
    initLogger();
    ConfigReader::readConfig();

    trader::HftTrader trader;
    trader.start();
  } catch (const std::exception &e) {
    std::cerr << e.what() << '\n';
  }

  return 0;
}