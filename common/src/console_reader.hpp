/**
 * @author Vladimir Pavliv
 * @date 2025-02-13
 */

#ifndef HFT_COMMON_CONSOLEREADER_HPP
#define HFT_COMMON_CONSOLEREADER_HPP

#include <fcntl.h>
#include <iostream>
#include <unistd.h>

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

  ConsoleReader(SystemBus &bus) : bus_{bus}, timer_{bus_.ioCtx} {
    fcntl(STDIN_FILENO, F_SETFL, O_NONBLOCK);
    std::cout << std::unitbuf;
  }

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
    timer_.async_wait([this](BoostErrorCode ec) {
      if (ec) {
        return;
      }
      inputCheck();
      scheduleInputCheck();
    });
  }

  void inputCheck() {
    struct pollfd fds = {STDIN_FILENO, POLLIN, 0};
    if (poll(&fds, 1, 0) == 1) {
      String input;
      std::getline(std::cin, input);
      if (!input.empty()) {
        Parser::parse(input, bus_);
      }
    }
  }

private:
  SystemBus &bus_;
  SteadyTimer timer_;
};

} // namespace hft

#endif // HFT_COMMON_CONSOLEREADER_HPP
