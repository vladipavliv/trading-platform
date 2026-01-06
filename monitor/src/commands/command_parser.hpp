/**
 * @author Vladimir Pavliv
 * @date 2025-04-10
 */

#ifndef HFT_MONITOR_COMMANDPARSER_HPP
#define HFT_MONITOR_COMMANDPARSER_HPP

#include <map>

#include "boost_types.hpp"
#include "bus/busable.hpp"
#include "client/src/commands/command.hpp"
#include "client/src/commands/command_parser.hpp"
#include "command.hpp"
#include "logging.hpp"
#include "server/src/commands/command.hpp"
#include "server/src/commands/command_parser.hpp"
#include "types.hpp"

namespace hft::monitor {

/**
 * @brief Parser/Serializer for MonitorCommand as well as Server/Client commands
 * MonitorCommand for native control, Server/Client commands to send them over the kafka
 */
class CommandParser {
public:
  static const std::map<String, Command> commands;

  static bool parse(CRef<String> cmd, Busable auto &consumer) {
    bool ret{false};
    if (client::CommandParser::parse(cmd, consumer) | server::CommandParser::parse(cmd, consumer)) {
      ret = true;
    }
    const auto cmdIt = commands.find(cmd);
    if (cmdIt != commands.end()) {
      consumer.post(cmdIt->second);
      ret = true;
    }
    return ret;
  }

  static bool deserialize(const uint8_t *data, size_t size, Busable auto &consumer) {
    return parse(String(data, size), consumer);
  }

  static ByteBuffer serialize(CRef<Command> cmd) {
    const auto it = std::find_if(commands.begin(), commands.end(),
                                 [cmd](const auto &element) { return element.second == cmd; });
    if (it == commands.end()) {
      LOG_ERROR("Unknown command {}", utils::toString(cmd));
      return ByteBuffer{};
    }
    return ByteBuffer{it->first.begin(), it->first.end()};
  }

  static ByteBuffer serialize(CRef<server::Command> cmd) {
    return server::CommandParser::serialize(cmd);
  }

  static ByteBuffer serialize(CRef<client::Command> cmd) {
    return client::CommandParser::serialize(cmd);
  }
};

inline const std::map<String, Command> CommandParser::commands{{"q", Command::Shutdown}};

} // namespace hft::monitor

#endif // HFT_SERVER_COMMANDPARSER_HPP
