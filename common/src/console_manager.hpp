/**
 * @author Vladimir Pavliv
 * @date 2025-02-13
 */

#ifndef HFT_COMMON_CONSOLEMANAGER_HPP
#define HFT_COMMON_CONSOLEMANAGER_HPP

#include <map>

#include "boost_types.hpp"
#include "bus/system_bus.hpp"
#include "logger.hpp"
#include "types.hpp"

namespace hft {

/**
 * @brief Periodically checks console input and posts commands to EventBus
 */
template <typename CommandType>
class ConsoleManager {
public:
  using Command = CommandType;

  ConsoleManager(SystemBus &bus) : bus_{bus}, timer_{bus_.systemIoCtx} { commands_.reserve(20); }

  void addCommand(CRefString input, Command command) { commands_.emplace(input, command); }

  void start() { scheduleInputCheck(); }

  void stop() { timer_.cancel(); }

  void printCommands() {
    Logger::monitorLogger->info("Commands:");
    for (auto &command : commands_) {
      Logger::monitorLogger->info("> {:3} => {}", command.first, utils::toString(command.second));
    }
  }

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
    bus_.publish(iterator->second);
  }

private:
  SystemBus &bus_;
  SteadyTimer timer_;
  std::unordered_map<std::string, CommandType> commands_;
};

} // namespace hft

#endif // HFT_COMMON_CONSOLEMANAGER_HPP
