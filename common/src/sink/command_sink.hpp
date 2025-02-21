/**
 * @author Vladimir Pavliv
 * @date 2025-02-13
 */

#ifndef HFT_COMMON_COMMAND_SINK_HPP
#define HFT_COMMON_COMMAND_SINK_HPP

#include <functional>
#include <map>

#include "template_types.hpp"
#include "utils/utils.hpp"

namespace hft {

/**
 * @brief Sink for synchronous message exchange. Handles commands from console
 * Ojbects subscribe to commands and events, which are being registered in template list
 * Posting certain command sends request for event like collect stats about traffic
 */
template <typename CommandType, typename... EventTypes>
class ControlSink {
public:
  using Command = CommandType;
  using CommandHandler = std::function<void(Command)>;

  void start() {}
  void stop() {}

  void addCommandHandler(const std::vector<Command> &commands, CommandHandler handler) {
    for (auto command : commands) {
      mHandlers[command].push_back(handler);
    }
  }

  template <typename EventType>
    requires(std::disjunction_v<std::is_same<EventType, EventTypes>...>)
  void addEventHandler(CRefHandler<EventType> &&handler) {
    getEventHandlers<EventType>().push_back(std::move(handler));
  }

  void onCommand(Command command) {
    auto cmdHandlers = mHandlers.find(command);
    if (cmdHandlers == mHandlers.end()) {
      return;
    }
    for (const auto &handler : cmdHandlers->second) {
      handler(command);
    }
  }

  template <typename EventType>
    requires(std::disjunction_v<std::is_same<EventType, EventTypes>...>)
  void onEvent(const EventType &event) {
    for (auto &handler : getEventHandlers<EventType>()) {
      handler(event);
    }
  }

private:
  template <typename EventType>
  std::vector<CRefHandler<EventType>> &getEventHandlers() {
    constexpr auto index = utils::getTypeIndex<EventType, EventTypes...>();
    return std::get<index>(mEventHandlers);
  }

private:
  std::map<Command, std::vector<CommandHandler>> mHandlers;
  std::tuple<std::vector<CRefHandler<EventTypes>>...> mEventHandlers;
};

} // namespace hft

#endif // HFT_COMMON_COMMAND_SINK_HPP