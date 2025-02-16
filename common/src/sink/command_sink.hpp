/**
 *
 * @author Vladimir Pavliv
 * @date 2025-02-13
 */

#ifndef HFT_COMMON_COMMAND_SINK_HPP
#define HFT_COMMON_COMMAND_SINK_HPP

#include <functional>
#include <map>

namespace hft {

template <typename CommandType>
class CommandSink {
public:
  using Command = CommandType;
  using Handler = std::function<void(Command)>;

  void start() {}
  void stop() {}

  void setHandler(Command command, Handler &&handler) {
    mHandlers[command].push_back(std::move(handler));
  }

  void post(Command command) {
    auto commands = mHandlers.find(command);
    if (commands == mHandlers.end()) {
      return;
    }
    for (const auto &handler : commands->second) {
      handler(command);
    }
  }

private:
  std::map<Command, std::vector<Handler>> mHandlers;
};

} // namespace hft

#endif // HFT_COMMON_COMMAND_SINK_HPP