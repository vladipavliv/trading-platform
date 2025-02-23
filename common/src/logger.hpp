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

/**
 * @brief By switching to a monitoring mode main logger gets disabled
 * and logs written via logService get to console via a separate logger
 */
class Logger {
public:
  static const std::string kPattern;

  using LogLevel = spdlog::level::level_enum;
  using SPtrSpdLogger = std::shared_ptr<spdlog::logger>;

  static SPtrSpdLogger mainLogger;
  static SPtrSpdLogger monitorLogger;

  static void initialize(LogLevel logLvl, const std::string &fileName = "") {
    initMonitorLogger();
    initMainLogger(fileName);
    spdlog::set_level(logLvl);
    spdlog::flush_on(spdlog::level::err);
  }

  static void switchLogLevel(bool goUp) {
    using namespace spdlog::level;
    level_enum currentLvl = spdlog::get_level();
    switch (currentLvl) {
    case level_enum::trace:
      spdlog::set_level(goUp ? level_enum::trace : level_enum::debug);
      break;
    case level_enum::debug:
      spdlog::set_level(goUp ? level_enum::trace : level_enum::info);
      break;
    case level_enum::info:
      spdlog::set_level(goUp ? level_enum::debug : level_enum::warn);
      break;
    case level_enum::warn:
      spdlog::set_level(goUp ? level_enum::info : level_enum::err);
      break;
    case level_enum::err:
      spdlog::set_level(goUp ? level_enum::warn : level_enum::critical);
      break;
    case level_enum::critical:
      spdlog::set_level(goUp ? level_enum::err : level_enum::critical);
      break;
    case level_enum::off:
      // monitor mode
      return;
    default:
      break;
    }
  }

private:
  static void initMonitorLogger() {
    auto consoleSink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    monitorLogger = std::make_shared<spdlog::logger>("console_monitor", consoleSink);
    monitorLogger->set_pattern(kPattern);
  }

  static void initMainLogger(const std::string &filename) {
    spdlog::init_thread_pool(8192, 1);
    auto rotatingSink =
        std::make_shared<spdlog::sinks::rotating_file_sink_mt>(filename, 10 * 1024 * 1024, 5);
    auto kMainLogger = std::make_shared<spdlog::async_logger>(
        "async_file_logger", rotatingSink, spdlog::thread_pool(),
        spdlog::async_overflow_policy::overrun_oldest);
    kMainLogger->set_pattern(kPattern);
    spdlog::register_logger(kMainLogger);
    spdlog::set_default_logger(kMainLogger);
  }
};

Logger::SPtrSpdLogger Logger::mainLogger{};
Logger::SPtrSpdLogger Logger::monitorLogger{};

const std::string Logger::kPattern = std::string{"%H:%M:%S.%f [%^%L%$] %v"};

} // namespace hft

#endif // HFT_COMMON_LOGGER_HPP