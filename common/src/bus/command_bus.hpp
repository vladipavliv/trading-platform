/**
 * @author Vladimir Pavliv
 * @date 2025-02-13
 */

#ifndef HFT_COMMON_COMMANDBUS_HPP
#define HFT_COMMON_COMMANDBUS_HPP

#include <functional>
#include <map>
#include <typeinfo>

#include "template_types.hpp"
#include "types.hpp"
#include "utils/utils.hpp"

namespace hft {

/**
 * @brief Bus to establish direct command flow between objects
 * @details As command flow is not required to be hot, each command may have multiple handlers
 */
template <typename CommandType>
class CommandBus {
public:
  using Command = CommandType;

  CommandBus() = default;

  void subscribe(Command command, Callback callback) {
    auto &cmdCallbacks = callbacks_[command];
    if (cmdCallbacks.empty()) {
      cmdCallbacks.reserve(10);
    }
    cmdCallbacks.emplace_back(std::move(callback));
  }

  void publish(Command command) {
    auto &cmdCallbacks = callbacks_[command];
    for (auto &callback : cmdCallbacks) {
      callback();
    }
  }

private:
  CommandBus(const CommandBus &) = delete;
  CommandBus(CommandBus &&) = delete;
  CommandBus &operator=(const CommandBus &) = delete;
  CommandBus &operator=(const CommandBus &&) = delete;

  HashMap<CommandType, std::vector<Callback>> callbacks_;
};

} // namespace hft

#endif // HFT_COMMON_COMMANDBUS_HPP