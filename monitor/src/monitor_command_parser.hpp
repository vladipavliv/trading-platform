/**
 * @author Vladimir Pavliv
 * @date 2025-04-10
 */

#ifndef HFT_MONITOR_COMMANDPARSER_HPP
#define HFT_MONITOR_COMMANDPARSER_HPP

#include <map>

#include "boost_types.hpp"
#include "bus/busable.hpp"
#include "commands/client_command.hpp"
#include "commands/client_command_parser.hpp"
#include "commands/server_command.hpp"
#include "commands/server_command_parser.hpp"
#include "logging.hpp"
#include "monitor_command.hpp"
#include "types.hpp"

namespace hft::monitor {

/**
 * @brief Parser/Serializer for MonitorCommand as well as Server/Client commands
 * MonitorCommand for native control, Server/Client commands to send them over the kafka
 */
class MonitorCommandParser {
public:
  static const std::map<String, MonitorCommand> commands;

  template <Busable Consumer>
  static bool parse(CRef<String> cmd, Consumer &consumer) {
    bool ret{false};
    if (client::ClientCommandParser::parse(cmd, consumer) |
        server::ServerCommandParser::parse(cmd, consumer)) {
      ret = true;
    }
    const auto cmdIt = commands.find(cmd);
    if (cmdIt != commands.end()) {
      consumer.post(cmdIt->second);
      ret = true;
    }
    return ret;
  }

  /**
   * @brief Interface for usage as a serializer when simple string map is sufficient
   */
  template <Busable Consumer>
  static bool deserialize(const uint8_t *data, size_t size, Consumer &consumer) {
    return parse(String(data, size), consumer);
  }

  static ByteBuffer serialize(CRef<MonitorCommand> cmd) {
    const auto it = std::find_if(commands.begin(), commands.end(),
                                 [cmd](const auto &element) { return element.second == cmd; });
    if (it == commands.end()) {
      LOG_ERROR("Unknown command {}", utils::toString(cmd));
      return ByteBuffer{};
    }
    return ByteBuffer{it->first.begin(), it->first.end()};
  }

  static ByteBuffer serialize(CRef<server::ServerCommand> cmd) {
    return server::ServerCommandParser::serialize(cmd);
  }

  static ByteBuffer serialize(CRef<client::ClientCommand> cmd) {
    return client::ClientCommandParser::serialize(cmd);
  }
};

inline const std::map<String, MonitorCommand> MonitorCommandParser::commands{
    {"q", MonitorCommand::Shutdown}};

} // namespace hft::monitor

#endif // HFT_SERVER_COMMANDPARSER_HPP
