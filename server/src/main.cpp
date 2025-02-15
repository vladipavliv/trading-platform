#include "config/config.hpp"
#include "config/config_reader.hpp"
#include "hft_server.hpp"
#include "init_logger.hpp"

int main() {
  using namespace hft;
  initLogger();

  ConfigReader::readConfig();
  // Config::logConfig(Config::cfg);
  server::HftServer server;
  server.start();
  return 0;
}