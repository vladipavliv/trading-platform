/**
 * @author Vladimir Pavliv
 * @date 2025-02-13
 */

#include <iostream>

#include "client_control_center.hpp"
#include "config/client_config.hpp"
#include "logging.hpp"

int main(int argc, char *argv[]) {
  using namespace hft;
  using namespace client;
  try {
    ClientConfig::load("client_config.ini");
    LOG_INIT(ClientConfig::cfg.logOutput);

    ClientControlCenter clientCc;
    clientCc.start();
  } catch (const std::exception &e) {
    std::cerr << "Exception caught in main " << e.what() << std::endl;
  } catch (...) {
    std::cerr << "Unknown exception caught in main" << std::endl;
  }
  return 0;
}
