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
#include <memory>
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
    printCommands();
  }

  Result<Command> getCommand() {
    grabInput();
    auto idx = mInput.find('\n');
    if (idx == std::string::npos) {
      return ErrorCode::Empty;
    }
    std::string commandStr = mInput.substr(0, idx);
    mInput.erase(0, idx);

    auto commandIter = mCommandMap.find(commandStr);
    if (commandIter == mCommandMap.end()) {
      // Invalid input, try next line
      return getCommand();
    }
    return commandIter->second;
  }

private:
  void grabInput() {
    if (std::cin.rdbuf()->in_avail() == 0) {
      return;
    }
    char buffer[256];
    std::cin.readsome(buffer, sizeof(buffer));
    mInput.append(buffer, std::cin.gcount());
  }

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
      spdlog::info(std::format("{:<{}}{}", ss.str(), 40, // TODO(do) Fix later
                               utils::toString(static_cast<uint8_t>(inpVec.first))));
    }
  }

private:
  std::string mInput;
  std::map<std::string, Command> mCommandMap;
};

} // namespace hft

#endif // HFT_COMMON_CONSOLE_INPUT_PARSER_HPP