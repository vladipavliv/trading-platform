/**
 * @author Vladimir Pavliv
 * @date 2025-02-13
 */

#include <iostream>

#include "config/trader_config.hpp"
#include "config/trader_config_reader.hpp"
#include "logging.hpp"
#include "trader_control_center.hpp"

int main(int argc, char *argv[]) {
  using namespace hft;
  using namespace trader;
  try {
    TraderConfigReader::readConfig("trader_config.ini");
    LOG_INIT(TraderConfig::cfg.logOutput);

    TraderControlCenter traderCc;
    traderCc.start();
  } catch (const std::exception &e) {
    std::cerr << "Exception caught in main " << e.what() << std::endl;
  } catch (...) {
    std::cerr << "Unknown exception caught in main" << std::endl;
  }
  return 0;
}
