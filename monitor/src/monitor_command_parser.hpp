/**
 * @author Vladimir Pavliv
 * @date 2025-04-10
 */

#ifndef HFT_MONITOR_COMMANDPARSER_HPP
#define HFT_MONITOR_COMMANDPARSER_HPP

#include "boost_types.hpp"
#include "logging.hpp"
#include "monitor_command.hpp"
#include "template_types.hpp"
#include "types.hpp"

namespace hft::monitor {

/**
 * @brief
 */
class MonitorCommandParser {
public:
  static const HashMap<String, MonitorCommand> commands;

  template <typename Consumer>
  static bool parse(CRef<String> cmd, Consumer &&consumer) {
    const auto cmdIt = commands.find(cmd);
    if (cmdIt == commands.end()) {
      LOG_ERROR("Command not found {}", cmd);
      return false;
    }
    consumer.post(cmdIt->second);
    return true;
  }

  /**
   * @brief Interface for usage as a serializer when simple string map is sufficient
   */
  static ByteBuffer serialize(MonitorCommand cmd) {
    const auto it = std::find_if(commands.begin(), commands.end(),
                                 [cmd](const auto &element) { return element.second == cmd; });
    if (it == commands.end()) {
      LOG_ERROR("Unknown command {}", utils::toString(cmd));
      return ByteBuffer{};
    }
    return ByteBuffer{it->first.begin(), it->first.end()};
  }

  template <typename Consumer>
  static bool deserialize(const uint8_t *data, size_t size, Consumer &&consumer) {
    return parse(String(data, size), std::forward<Consumer>(consumer));
  }
};

const HashMap<String, MonitorCommand> MonitorCommandParser::commands{
    {"sp+", MonitorCommand::ServerPriceFeedStart},  {"sp-", MonitorCommand::ServerPriceFeedStop},
    {"sk+", MonitorCommand::ServerKafkaFeedStart},  {"sk-", MonitorCommand::ServerKafkaFeedStop},
    {"sq", MonitorCommand::ServerShutdown},         {"tt+", MonitorCommand::TraderTradeStart},
    {"tt-", MonitorCommand::TraderTradeStop},       {"tts+", MonitorCommand::TraderTradeSpeedUp},
    {"tts-", MonitorCommand::TraderTradeSpeedDown}, {"tq", MonitorCommand::TraderShutdown}};

} // namespace hft::monitor

#endif // HFT_SERVER_COMMANDPARSER_HPP