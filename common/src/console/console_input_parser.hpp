/**
 * @file monitor.hpp
 * @brief
 *
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

#include "error_code.hpp"
#include "result.hpp"
#include "sink/command_sink.hpp"
#include "utils/string_utils.hpp"

namespace hft {

template <typename CommandType>
class ConsoleInputParser {
public:
  using Command = CommandType;

  ConsoleInputParser(std::map<std::string, Command> &&cmdMap) : mCommandMap{std::move(cmdMap)} {
    fcntl(STDIN_FILENO, F_SETFL, O_NONBLOCK);
    std::cout << std::unitbuf;

    std::string header = "Available commands:";
    spdlog::info("> {} {}", header, std::string(mCommandsWidth - header.length() - 1, '~'));
    printCommands();
    spdlog::info("> {}", std::string(mCommandsWidth, '~'));
    spdlog::info("");
  }

  Result<Command> getCommand() {
    if (!inputAvailable()) {
      return ErrorCode::Error;
    }
    std::getline(std::cin, mInput);
    auto cmdIt = mCommandMap.find(mInput);
    mInput.clear();
    if (cmdIt == mCommandMap.end()) {
      return ErrorCode::Empty;
    } else {
      return cmdIt->second;
    }
    return ErrorCode::Empty;
  }

  Result<Command> tryGetCommand() {}

private:
  bool inputAvailable() {
    struct pollfd fds = {STDIN_FILENO, POLLIN, 0};
    return poll(&fds, 1, 0) == 1;
  }

  void printCommands() {
    static std::map<Command, std::vector<std::string>> cmdToInput;
    if (cmdToInput.empty()) {
      for (auto &cmd : mCommandMap) {
        cmdToInput[cmd.second].push_back(cmd.first);
      }
    }

    for (auto &inpVec : cmdToInput) {
      std::stringstream ss;
      ss << "> ";
      for (auto &cmd : inpVec.second) {
        ss << "\'" << cmd << "\' ";
      }
      auto cmdStr = ss.str();
      auto descrStr = utils::toString(inpVec.first);
      spdlog::info(std::format(
          "{:<} {:^} {:>}", cmdStr,
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
