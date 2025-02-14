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

#include "sink/command_sink.hpp"
#include "utils/string_utils.hpp"

namespace hft {

template <typename CommandType>
class ConsoleInputParser {
public:
  using Command = CommandType;
  using Sink = CommandSink<Command>;

  ConsoleInputParser(Sink &sink) : mSink{sink} { printCommands(); }

  void run() {
    std::string input;
    while (input != "q") {
      std::getline(std::cin, input);
      auto cmdString = utils::toLower(input);
      auto command = mCommandMap.find(cmdString);
      if (command != mCommandMap.end()) {
        mSink.post(command->second);
      } else {
        spdlog::error("Unknown command");
        printCommands();
      }
    }
  }

  void addCommand(String &&str, Command cmd) {
    mCommandMap.emplace({std::forward<String>(str), cmd});
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
      spdlog::info(std::format("{:<{}}{}", ss.str(), 40, // TODO(do) Fix later
                               utils::toString(static_cast<uint8_t>(inpVec.first))));
    }
  }

private:
  Sink &mSink;
  std::map<std::string, Command> mCommandMap;
};

} // namespace hft

#endif // HFT_COMMON_CONSOLE_INPUT_PARSER_HPP