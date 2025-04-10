/**
 * @author Vladimir Pavliv
 * @date 2025-04-10
 */

#ifndef HFT_SERVER_COMMANDPARSER_HPP
#define HFT_SERVER_COMMANDPARSER_HPP

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
  template <typename Consumer>
  static bool deserialize(const uint8_t *data, size_t size, Consumer &&consumer) {
    return deserialize(String(data, size), std::forward<Consumer>(consumer));
  }

  template <typename Consumer>
  static bool deserialize(CRef<String> cmd, Consumer &&consumer) {
    static const HashMap<String, ServerCommand> commands{{"p+", ServerCommand::PriceFeedStart},
                                                         {"p-", ServerCommand::PriceFeedStop},
                                                         {"k+", ServerCommand::KafkaFeedStart},
                                                         {"k-", ServerCommand::KafkaFeedStop},
                                                         {"q", ServerCommand::Shutdown}};
    const auto cmdIt = commands.find(cmd);
    if (cmdIt == commands.end()) {
      LOG_ERROR("Command not found {}", cmd);
      return false;
    }
    consumer.post(cmdIt->second);
    return true;
  }
};

} // namespace hft::server

#endif // HFT_SERVER_COMMANDPARSER_HPP