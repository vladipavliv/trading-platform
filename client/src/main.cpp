/**
 * @author Vladimir Pavliv
 * @date 2025-02-13
 */

#include <iostream>

#include <boost/program_options.hpp>

#include "client_control_center.hpp"
#include "config/client_config.hpp"
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
