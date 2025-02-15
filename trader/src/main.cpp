#include "config/config.hpp"
#include "config/config_reader.hpp"
#include "hft_trader.hpp"
#include "init_logger.hpp"

int main() {
  using namespace hft;
  initLogger();

  ConfigReader::readConfig();
  Config::logTraderConfig(Config::cfg.trader);

  trader::HftTrader trader;
  trader.start();
  return 0;
}