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
#include "bus/bus_restrictor.hpp"
#include "network/shm/shm_server.hpp"

#include "config/server_config.hpp"
#include "control_center.hpp"
#include "logging.hpp"

int main(int argc, char *argv[]) {
  using namespace hft;
  using namespace server;
  using namespace boost;

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
    ServerConfig::load(configPath);
    LOG_INIT(ServerConfig::cfg.logOutput);

    ServerControlCenter serverCc;
    serverCc.start();
  } catch (const std::exception &e) {
    std::cerr << "Exception caught in main " << e.what() << std::endl;
  }
  return 0;
}
