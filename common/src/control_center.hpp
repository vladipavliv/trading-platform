/**
 * @author Vladimir Pavliv
 * @date 2025-02-13
 */

#ifndef HFT_COMMON_CONTROLCENTER_HPP
#define HFT_COMMON_CONTROLCENTER_HPP

#include <map>

#include "boost_types.hpp"
#include "event_bus.hpp"
#include "logger.hpp"
#include "types.hpp"

namespace hft {

template <typename CommandType>
class ControlCenter {
public:
  using Command = CommandType;

  ControlCenter(IoContext &ctx) : timer_{ctx} { commands_.reserve(20); }

  void addCommand(CRefString input, Command command) { commands_.emplace(input, command); }

  void start() {
    printCommands();
    scheduleInputCheck();
  }

  void stop() { timer_.cancel(); }

private:
  void scheduleInputCheck() {
    timer_.expires_after(Milliseconds(200));
    timer_.async_wait([this](BoostErrorRef ec) {
      if (ec) {
        return;
      }
      inputCheck();
      scheduleInputCheck();
    });
  }

  void inputCheck() {
    auto input = utils::getConsoleInput();
    const auto &iterator = commands_.find(input);
    if (iterator == commands_.end()) {
      return;
    }
    EventBus::bus().publish<CommandType>(iterator->second);
  }

  void printCommands() {
    Logger::monitorLogger->info("Commands:");
    for (auto &command : commands_) {
      Logger::monitorLogger->info("> {:5} => {}", command.first, utils::toString(command.second));
    }
  }

private:
  SteadyTimer timer_;
  std::unordered_map<std::string, CommandType> commands_;
};

} // namespace hft

#endif // HFT_COMMON_CONTROLCENTER_HPP