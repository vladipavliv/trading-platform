/**
 * @author Vladimir Pavliv
 * @date 2025-02-13
 */

#ifndef HFT_COMMON_CONSOLEREADER_HPP
#define HFT_COMMON_CONSOLEREADER_HPP

#include <map>

#include "boost_types.hpp"
#include "bus/system_bus.hpp"
#include "logging.hpp"
#include "types.hpp"

namespace hft {

/**
 * @brief Performs non blocking console input check at a certain rate
 * Checks the input over the commands map and and posts parsed commands to the system bus
 */
template <typename ParserType>
class ConsoleReader {
public:
  using Parser = ParserType;

  ConsoleReader(SystemBus &bus) : bus_{bus}, timer_{bus_.ioCtx} { utils::unblockConsole(); }

  void start() { scheduleInputCheck(); }

  void stop() { timer_.cancel(); }

  void printCommands() {
    LOG_INFO_SYSTEM("Commands:");
    for (auto &command : Parser::commands) {
      LOG_INFO_SYSTEM("> {:3} => {}", command.first, utils::toString(command.second));
    }
  }

private:
  void scheduleInputCheck() {
    timer_.expires_after(Milliseconds(200));
    timer_.async_wait([this](CRef<BoostError> ec) {
      if (ec) {
        return;
      }
      inputCheck();
      scheduleInputCheck();
    });
  }

  void inputCheck() {
    const auto input = utils::getConsoleInput();
    if (!input.empty()) {
      Parser::parse(input, bus_);
    }
  }

private:
  SystemBus &bus_;
  SteadyTimer timer_;
};

} // namespace hft

#endif // HFT_COMMON_CONSOLEREADER_HPP
