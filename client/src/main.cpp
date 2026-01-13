/**
 * @author Vladimir Pavliv
 * @date 2025-02-13
 */

#include <iostream>

#include <boost/program_options.hpp>

#include "adapters/telemetry_adapter.hpp"
#ifdef COMM_SHM
#include "transport/shm/shm_client.hpp"
#include "trusted_connection_manager.hpp"
#else
#include "transport/boost/boost_network_client.hpp"

#include "network_connection_manager.hpp"
#endif

#include "adapters/dummies/dummy_kafka_adapter.hpp"
#include "adapters/kafka/kafka_adapter.hpp"
#include "adapters/postgres/postgres_adapter.hpp"
#include "bus/bus_hub.hpp"
#include "transport/shm/shm_manager.hpp"

#include "config/client_config.hpp"
#include "control_center.hpp"
#include "logging.hpp"
#include "utils/time_utils.hpp"

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
    ClientConfig::cfg.nsPerCycle = utils::getNsPerCycle();

#ifdef COMM_SHM
    if (ClientConfig::cfg.coresApp.size() > 1) {
      throw std::logic_error("Multi-worker currently not supported for shm");
    }
#endif

    ShmManager::initialize(false);

    ClientControlCenter clientCc;
    clientCc.start();
  } catch (const std::exception &e) {
    std::cerr << "Exception caught in main " << e.what() << std::endl;
  } catch (...) {
    std::cerr << "Unknown exception caught in main" << std::endl;
  }
  ShmManager::deinitialize();
  return 0;
}
