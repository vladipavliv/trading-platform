#include "config/config.hpp"
#include "hft_server.hpp"
#include "init_logger.hpp"

int main() {
  using namespace hft;
  try {
    initLogger();
    Config::logServerConfig(Config::config().server);
    server::HftServer server;
    server.start();
  } catch (const std::exception &ex) {
    spdlog::error("Exception in HftServer: {}", ex.what());
  }
  return 0;
}