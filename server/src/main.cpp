/**
 * @author Vladimir Pavliv
 * @date 2025-02-13
 */

#include <iostream>

#include <boost/program_options.hpp>

#include "adapters/telemetry_adapter.hpp"
#include "execution/orderbook/flat_order_book.hpp"
#ifdef COMM_SHM
#include "session/trusted_session_manager.hpp"
#include "transport/shm/shm_server.hpp"
#else
#include "session/network_session_manager.hpp"
#include "transport/boost/boost_network_server.hpp"
#endif

#include "adapters/dummies/dummy_kafka_adapter.hpp"
#include "adapters/kafka/kafka_adapter.hpp"
#include "adapters/postgres/postgres_adapter.hpp"
#include "bus/bus_hub.hpp"
#include "bus/bus_restrictor.hpp"

#include "config/server_config.hpp"
#include "control_center.hpp"
#include "logging.hpp"
#include "utils/time_utils.hpp"

int main(int argc, char *argv[]) {
  using namespace hft;
  using namespace server;
  using namespace boost;
  using namespace utils;

  std::string configPath;

  program_options::options_description desc("Allowed options");
  desc.add_options()("help,h", "produce help message")(
      "config,c", program_options::value<String>(&configPath)->default_value("server_config.ini"),
      "Path to config file");

  program_options::variables_map varmMap;
  program_options::store(program_options::parse_command_line(argc, argv, desc), varmMap);
  program_options::notify(varmMap);

  if (varmMap.count("help")) {
    std::cout << desc << std::endl;
    return 0;
  }

  try {
    ServerConfig::cfg().load(configPath);
    LOG_INIT(ServerConfig::cfg().logOutput);

#ifdef COMM_SHM
    if (ServerConfig::cfg().coresApp.size() > 1) {
      throw std::logic_error("Multi-worker currently not supported for shm");
    }
#endif

    ControlCenter cc;
    cc.start();
  } catch (const std::exception &e) {
    std::cerr << "Exception caught in main " << e.what() << std::endl;
  }

  return 0;
}
