/**
 * @author Vladimir Pavliv
 * @date 2025-02-10
 */

#ifndef HFT_COMMON_CONSOLE_INPUT_PARSER_HPP
#define HFT_COMMON_CONSOLE_INPUT_PARSER_HPP

#include <fcntl.h>
#include <format>
#include <iostream>
#include <map>
#include <spdlog/spdlog.h>
#include <string>
#include <sys/select.h>
#include <unistd.h>

#include "result.hpp"
#include "sink/command_sink.hpp"
#include "status_code.hpp"
#include "utils/string_utils.hpp"

namespace hft {

/**
 * @brief Reads and parses commands from console in non blocking way
 */
template <typename CommandType>
class ConsoleInputParser {
public:
  using Command = CommandType;

  ConsoleInputParser(std::map<std::string, Command> &&cmdMap) : mCommandMap{std::move(cmdMap)} {
    fcntl(STDIN_FILENO, F_SETFL, O_NONBLOCK);
    std::cout << std::unitbuf;
    printCommands();
  }

  bool waitForInput(uint32_t timeoutMs = 0) {
    struct pollfd fds = {STDIN_FILENO, POLLIN, 0};
    return poll(&fds, 1, timeoutMs) == 1;
  }

  Result<Command> getCommand(uint32_t timeoutMs = 0) {
    if (!waitForInput(timeoutMs)) {
      return StatusCode::Empty;
    }
    std::getline(std::cin, mInput);
    auto cmdIt = mCommandMap.find(mInput);
    mInput.clear();
    if (cmdIt == mCommandMap.end()) {
      return StatusCode::Empty;
    } else {
      return cmdIt->second;
    }
    return StatusCode::Empty;
  }

private:
  void printCommands() {
    static std::map<Command, std::vector<std::string>> cmdToInput;
    if (cmdToInput.empty()) {
      for (auto &cmd : mCommandMap) {
        cmdToInput[cmd.second].push_back(cmd.first);
      }
    }

    for (auto &inpVec : cmdToInput) {
      std::stringstream ss;
      for (auto &cmd : inpVec.second) {
        ss << "\'" << cmd << "\' ";
      }
      auto cmdStr = ss.str();
      auto descrStr = utils::toString(inpVec.first);
      spdlog::info(std::format(
          "> {:<} {:^} {:>}", cmdStr,
          std::string(mCommandsWidth - cmdStr.length() - descrStr.length() - 1, '.'), descrStr));
    }
  }

private:
  std::string mInput;
  std::map<std::string, Command> mCommandMap;
  const uint8_t mCommandsWidth{60};
};

} // namespace hft

#endif // HFT_COMMON_CONSOLE_INPUT_PARSER_HPP
