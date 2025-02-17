/**
 * @author Vladimir Pavliv
 * @date 2025-02-13
 */

#include "config/config.hpp"
#include "config/config_reader.hpp"
#include "hft_server.hpp"
#include "init_logger.hpp"

int main() {
  try {
    using namespace hft;
    // initAsyncLogger("hft_server.txt");
    initLogger();
    ConfigReader::readConfig();
    Config::cfg.log();

    server::HftServer server;
    server.start();
  } catch (const std::exception &e) {
    std::cerr << e.what() << '\n';
  }
  return 0;
}