/**
 * @author Vladimir Pavliv
 * @date 2025-04-10
 */

#ifndef HFT_CLIENT_COMMANDPARSER_HPP
#define HFT_CLIENT_COMMANDPARSER_HPP

#include <map>

#include "bus/busable.hpp"
#include "command.hpp"
#include "logging.hpp"
#include "types.hpp"

namespace hft::client {

/**
 * @brief Maps strings to a client command and postst it to a consumer
 * For now incoming from kafka messages are simple strings so there is no need
 * of a full blown serializer.
 */
class CommandParser {
public:
  static const std::map<String, Command> commands;

  static bool parse(CRef<String> cmd, Busable auto &consumer) {
    const auto cmdIt = commands.find(cmd);
    if (cmdIt == commands.end()) {
      return false;
    }
    consumer.post(cmdIt->second);
    return true;
  }

  static bool deserialize(const uint8_t *data, size_t size, Busable auto &consumer) {
    return parse(String(reinterpret_cast<const char *>(data), size), consumer);
  }

  static ByteBuffer serialize(Command cmd) {
    const auto it = std::find_if(commands.begin(), commands.end(),
                                 [cmd](const auto &element) { return element.second == cmd; });
    if (it == commands.end()) {
      LOG_ERROR("Unknown command {}", utils::toString(cmd));
      return ByteBuffer{};
    }
    return ByteBuffer{it->first.begin(), it->first.end()};
  }
};

inline const std::map<String, Command> CommandParser::commands{

    {"s+", Command::Start},
    {"s-", Command::Stop},
    {"t+", Command::Telemetry_Start},
    {"t-", Command::Telemetry_Stop},
    {"q", Command::Shutdown}};

} // namespace hft::client

#endif // HFT_CLIENT_COMMANDPARSER_HPP