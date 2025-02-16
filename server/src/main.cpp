/**
 * @file
 * @brief
 *
 * @author Vladimir Pavliv
 * @date 2025-02-13
 */

#include "config/config.hpp"
#include "config/config_reader.hpp"
#include "hft_server.hpp"
#include "init_logger.hpp"

int main() {
  using namespace hft;
  initAsyncLogger("hft_server.txt");

  ConfigReader::readConfig();

  server::HftServer server;
  server.start();
  return 0;
}