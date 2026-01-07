/**
 * @author Vladimir Pavliv
 * @date 2025-02-13
 */

#include <iostream>

#include <boost/program_options.hpp>

#include "adapters/dummies/dummy_kafka_adapter.hpp"
#include "adapters/kafka/kafka_adapter.hpp"
#include "adapters/postgres/postgres_adapter.hpp"
#include "bus/bus_hub.hpp"

#ifdef COMM_SHM
#include "network/shm/shm_client.hpp"
#include "trusted_connection_manager.hpp"
#else
#include "network/boost/boost_network_client.hpp"
#include "network_connection_manager.hpp"
#endif

#include "config/client_config.hpp"
#include "control_center.hpp"
#include "logging.hpp"

int main(int argc, char *argv[]) {
  using namespace hft;
  using namespace client;
  using namespace boost;

  std::string configPath;

  program_options::options_description desc("Allowed options");
  desc.add_options()("help,h", "produce help message")(
      "config,c", program_options::value<String>(&configPath)->default_value("client_config.ini"),
      "Path to config file");

  program_options::variables_map varmMap;
  program_options::store(program_options::parse_command_line(argc, argv, desc), varmMap);
  program_options::notify(varmMap);

  if (varmMap.count("help")) {
    std::cout << desc << std::endl;
    return 0;
  }

  try {
    ClientConfig::load(configPath);
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
