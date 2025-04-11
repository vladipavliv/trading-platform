/**
 * @author Vladimir Pavliv
 * @date 2025-04-10
 */

#ifndef HFT_TRADER_COMMANDPARSER_HPP
#define HFT_TRADER_COMMANDPARSER_HPP

#include "logging.hpp"
#include "trader_command.hpp"
#include "types.hpp"

namespace hft::trader {

/**
 * @brief Maps strings to a trader command and postst it to a consumer
 * For now incoming from kafka messages are simple strings so there is no need
 * of a full blown serializer.
 */
class TraderCommandParser {
public:
  static const HashMap<String, TraderCommand> commands;

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
   * @brief Interface so it can be used as serializer when simple string map is sufficient
   */
  template <typename Consumer>
  static bool deserialize(const uint8_t *data, size_t size, Consumer &&consumer) {
    return parse(String(data, size), std::forward<Consumer>(consumer));
  }
};

const HashMap<String, TraderCommand> TraderCommandParser::commands{
    {"t+", TraderCommand::TradeStart},
    {"t-", TraderCommand::TradeStop},
    {"ts+", TraderCommand::TradeSpeedUp},
    {"ts-", TraderCommand::TradeSpeedDown},
    {"q", TraderCommand::Shutdown}};

} // namespace hft::trader

#endif // HFT_TRADER_COMMANDPARSER_HPP