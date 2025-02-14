/**
 * @file monitor.hpp
 * @brief
 *
 * @author Vladimir Pavliv
 * @date 2025-02-10
 */

#ifndef HFT_COMMON_CONSOLE_INPUT_PARSER_HPP
#define HFT_COMMON_CONSOLE_INPUT_PARSER_HPP

#include <format>
#include <iostream>
#include <map>
#include <spdlog/spdlog.h>
#include <string>

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
    std::cin.sync_with_stdio(false);
    printCommands();
  }

  Result<Command> getCommand() {
    // Get a character from the user without blocking
    char ch;
    while (std::cin.get(ch)) {
      if (ch != '\n') {
        mInput += ch;
        continue;
      }
      auto cmdIt = mCommandMap.find(mInput);
      mInput.clear();
      if (cmdIt == mCommandMap.end()) {
        return ErrorCode::Empty;
      } else {
        return cmdIt->second;
      }
    }
    return ErrorCode::Empty;
  }

private:
  void printCommands() {
    static std::map<Command, std::vector<std::string>> cmdToInput;
    if (cmdToInput.empty()) {
      for (auto &cmd : mCommandMap) {
        cmdToInput[cmd.second].push_back(cmd.first);
      }
    }

    spdlog::info("> Available commands: ");
    for (auto &inpVec : cmdToInput) {
      std::stringstream ss;
      ss << "> ";
      for (auto &cmd : inpVec.second) {
        ss << "\'" << cmd << "\' ";
      }
      auto cmdStr = ss.str();
      auto descrStr = utils::toString(inpVec.first);
      spdlog::info(
          std::format("{} {} {}", cmdStr, std::string(60 - cmdStr.length(), '.'), descrStr));
    }
  }

private:
  std::string mInput;
  std::map<std::string, Command> mCommandMap;
};

} // namespace hft

#endif // HFT_COMMON_CONSOLE_INPUT_PARSER_HPP