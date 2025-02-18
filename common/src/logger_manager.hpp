/**
 * @author Vladimir Pavliv
 * @date 2025-02-13
 */

#ifndef HFT_COMMON_LOGGER_HPP
#define HFT_COMMON_LOGGER_HPP

#include <memory>

#include <spdlog/async.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

#include "utils/string_utils.hpp"

namespace hft {

class LoggerManager {
public:
  static void switchLogLevel(bool switchUp) {
    using namespace spdlog::level;
    level_enum newLvl;
    level_enum currentLvl = spdlog::get_level();
    switch (currentLvl) {
    case level_enum::trace:
      newLvl = switchUp ? level_enum::trace : level_enum::debug;
      break;
    case level_enum::debug:
      newLvl = switchUp ? level_enum::trace : level_enum::info;
      break;
    case level_enum::info:
      newLvl = switchUp ? level_enum::debug : level_enum::warn;
      break;
    case level_enum::warn:
      newLvl = switchUp ? level_enum::info : level_enum::err;
      break;
    case level_enum::err:
      newLvl = switchUp ? level_enum::warn : level_enum::critical;
      break;
    case level_enum::critical:
      newLvl = switchUp ? level_enum::err : level_enum::critical;
      break;
    default:
      newLvl = currentLvl;
      break;
    }
    spdlog::set_level(newLvl);
    spdlog::flush_on(newLvl);
    spdlog::info("Log level: {}", utils::toString(newLvl));
  }

  static void initConsoleLogger(spdlog::level::level_enum logLvl) {
    auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    auto logger = std::make_shared<spdlog::logger>("console", console_sink);

    spdlog::set_default_logger(logger);
    spdlog::set_level(logLvl);
    spdlog::set_pattern("[%H:%M:%S.%e] [%^%L%$] %v");
    spdlog::flush_on(logLvl);
  }

  static void initAsyncLogger(spdlog::level::level_enum logLvl, const std::string &filename) {
    spdlog::init_thread_pool(8192, 1);
    auto async_file_logger = std::make_shared<spdlog::async_logger>(
        "async_file_logger", std::make_shared<spdlog::sinks::basic_file_sink_mt>(filename),
        spdlog::thread_pool());

    spdlog::set_level(logLvl);
    spdlog::set_default_logger(async_file_logger);
    spdlog::flush_on(logLvl);
  };

  static void initRotatingLogger(spdlog::level::level_enum logLvl, const std::string &filename) {
    auto rotating_logger = spdlog::rotating_logger_mt("rotating_logger", filename, 10485760, 3);

    spdlog::set_level(logLvl);
    spdlog::set_default_logger(rotating_logger);
  }
};

} // namespace hft

#endif // HFT_COMMON_