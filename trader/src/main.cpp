#include "config/config.hpp"
#include "config/config_reader.hpp"
#include "hft_trader.hpp"
#include "init_logger.hpp"

#include <boost/asio.hpp>

int main() {
  using namespace hft;
  initLogger();

  ConfigReader::readConfig();
  // Config::logConfig(Config::cfg);

  trader::HftTrader trader;
  trader.start();
  return 0;
}