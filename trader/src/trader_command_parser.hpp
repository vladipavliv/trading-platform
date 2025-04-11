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
    return parse(String(data, size), consumer);
  }

  static ByteBuffer serialize(TraderCommand cmd) {
    const auto it = std::find_if(commands.begin(), commands.end(),
                                 [cmd](const auto &element) { return element.second == cmd; });
    if (it == commands.end()) {
      LOG_ERROR("Unknown command {}", utils::toString(cmd));
      return ByteBuffer{};
    }
    return ByteBuffer{it->first.begin(), it->first.end()};
  }
};

const HashMap<String, TraderCommand> TraderCommandParser::commands{
    {"t+", TraderCommand::TradeStart},     {"t-", TraderCommand::TradeStop},
    {"ts+", TraderCommand::TradeSpeedUp},  {"ts-", TraderCommand::TradeSpeedDown},
    {"k+", TraderCommand::KafkaFeedStart}, {"k-", TraderCommand::KafkaFeedStop},
    {"q", TraderCommand::Shutdown}};

} // namespace hft::trader

#endif // HFT_TRADER_COMMANDPARSER_HPP