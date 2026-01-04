/**
 * @author Vladimir Pavliv
 * @date 2025-04-10
 */

#ifndef HFT_SERVER_COMMANDPARSER_HPP
#define HFT_SERVER_COMMANDPARSER_HPP

#include <map>

#include "bus/busable.hpp"
#include "logging.hpp"
#include "server_command.hpp"
#include "types.hpp"

namespace hft::server {

/**
 * @brief Maps strings to a server command and postst it to a consumer
 * For now incoming from kafka messages are simple strings so there is no need
 * of a full blown serializer.
 */
class ServerCommandParser {
public:
  static const std::map<String, ServerCommand> commands;

  template <Busable Consumer>
  static bool parse(CRef<String> cmd, Consumer &consumer) {
    const auto cmdIt = commands.find(cmd);
    if (cmdIt == commands.end()) {
      return false;
    }
    consumer.post(cmdIt->second);
    return true;
  }

  /**
   * @brief Interface so it can be used as serializer when simple string map is sufficient
   */
  template <Busable Consumer>
  static bool deserialize(const uint8_t *data, size_t size, Consumer &consumer) {
    return parse(String(reinterpret_cast<const char *>(data), size), consumer);
  }

  static ByteBuffer serialize(ServerCommand cmd) {
    const auto it = std::find_if(commands.begin(), commands.end(),
                                 [cmd](const auto &element) { return element.second == cmd; });
    if (it == commands.end()) {
      LOG_ERROR("Unknown command {}", utils::toString(cmd));
      return ByteBuffer{};
    }
    return ByteBuffer{it->first.begin(), it->first.end()};
  }
};

inline const std::map<String, ServerCommand> ServerCommandParser::commands{
    {"p+", ServerCommand::PriceFeed_Start},
    {"p-", ServerCommand::PriceFeed_Stop},
    {"t+", ServerCommand::Telemetry_Start},
    {"t-", ServerCommand::Telemetry_Stop},
    {"q", ServerCommand::Shutdown}};

} // namespace hft::server

#endif // HFT_SERVER_COMMANDPARSER_HPP