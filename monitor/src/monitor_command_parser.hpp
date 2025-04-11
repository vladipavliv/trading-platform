/**
 * @author Vladimir Pavliv
 * @date 2025-04-10
 */

#ifndef HFT_MONITOR_COMMANDPARSER_HPP
#define HFT_MONITOR_COMMANDPARSER_HPP

#include "boost_types.hpp"
#include "logging.hpp"
#include "monitor_command.hpp"
#include "server_command.hpp"
#include "server_command_parser.hpp"
#include "template_types.hpp"
#include "trader_command.hpp"
#include "trader_command_parser.hpp"
#include "types.hpp"

namespace hft::monitor {

/**
 * @brief Parser/Serializer for MonitorCommand as well as Server/Trader commands
 * MonitorCommand for native control, Server/Trader commands to send them over the kafka
 */
class MonitorCommandParser {
public:
  static const HashMap<String, MonitorCommand> commands;

  template <typename Consumer>
  static bool parse(CRef<String> cmd, Consumer &&consumer) {
    // first try to parse with native command map, then server/trader
    const auto cmdIt = commands.find(cmd);
    if (cmdIt != commands.end()) {
      consumer.post(cmdIt->second);
      return true;
    }
    if (server::ServerCommandParser::parse(cmd, consumer) ||
        trader::TraderCommandParser::parse(cmd, consumer)) {
      return true;
    }
    LOG_ERROR("Command not found {}", cmd);
    return false;
  }

  /**
   * @brief Interface for usage as a serializer when simple string map is sufficient
   */
  template <typename Consumer>
  static bool deserialize(const uint8_t *data, size_t size, Consumer &&consumer) {
    return parse(String(data, size), consumer);
  }

  static ByteBuffer serialize(MonitorCommand cmd) {
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

  static ByteBuffer serialize(CRef<trader::TraderCommand> cmd) {
    return trader::TraderCommandParser::serialize(cmd);
  }
};

const HashMap<String, MonitorCommand> MonitorCommandParser::commands{
    {"q", MonitorCommand::Shutdown}};

} // namespace hft::monitor

#endif // HFT_SERVER_COMMANDPARSER_HPP