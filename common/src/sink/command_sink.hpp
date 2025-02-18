/**
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

  void setHandler(Handler &&handler) { mHandlers.push_back(std::move(handler)); }

  void post(Command command) {
    for (auto &handler : mHandlers) {
      handler(command);
    }
  }

private:
  std::vector<Handler> mHandlers;
};

} // namespace hft

#endif // HFT_COMMON_COMMAND_SINK_HPP