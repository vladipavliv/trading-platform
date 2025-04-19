/**
 * @author Vladimir Pavliv
 * @date 2025-02-13
 */

#include <iostream>

#include "config/server_config.hpp"
#include "logging.hpp"
#include "server_control_center.hpp"

int main(int argc, char *argv[]) {
  using namespace hft;
  using namespace server;
  try {
    ServerConfig::load("server_config.ini");
    LOG_INIT(ServerConfig::cfg.logOutput);

    ServerControlCenter serverCc;
    serverCc.start();
  } catch (const std::exception &e) {
    std::cerr << "Exception caught in main " << e.what() << std::endl;
  }
  return 0;
}
