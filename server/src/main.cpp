#include "hft_server.hpp"
#include "init_logger.hpp"

int main() {
  try {
    hft::initLogger();
    hft::server::HftServer server;
    server.start();
  } catch (const std::exception &ex) {
    spdlog::error("Exception in HftServer: {}", ex.what());
  }
  return 0;
}