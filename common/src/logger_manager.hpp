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
class LoggerManager {
public:
  using LogLevel = spdlog::level::level_enum;
  using SPtrSpdLogger = std::shared_ptr<spdlog::logger>;

  enum class LoggerMode : uint8_t { Console = 0U, AsyncFile = 1U, RotatingFile = 2U };

  static void initialize(LoggerMode mode, LogLevel logLvl, const std::string &fileName = "") {
    switch (mode) {
    case LoggerMode::AsyncFile:
      initAsyncLogger(logLvl, fileName);
      break;
    case LoggerMode::RotatingFile:
      initRotatingLogger(logLvl, fileName);
      break;
    default:
      initConsoleLogger(logLvl);
      break;
    }
    initServiceLogger();
    toNormalMode();
    spdlog::set_level(logLvl);
    spdlog::flush_on(logLvl);
  }

  static inline void toMonitorMode() { spdlog::set_level(spdlog::level::off); }
  static inline void toNormalMode() { spdlog::set_level(kLogLevel); }
  static inline void logService(StringRef msg) { kMonitorLogger->info(msg); }

  static void switchLogLevel(bool goUp) {
    using namespace spdlog::level;
    level_enum currentLvl = spdlog::get_level();
    switch (currentLvl) {
    case level_enum::trace:
      kLogLevel = goUp ? level_enum::trace : level_enum::debug;
      break;
    case level_enum::debug:
      kLogLevel = goUp ? level_enum::trace : level_enum::info;
      break;
    case level_enum::info:
      kLogLevel = goUp ? level_enum::debug : level_enum::warn;
      break;
    case level_enum::warn:
      kLogLevel = goUp ? level_enum::info : level_enum::err;
      break;
    case level_enum::err:
      kLogLevel = goUp ? level_enum::warn : level_enum::critical;
      break;
    case level_enum::critical:
      kLogLevel = goUp ? level_enum::err : level_enum::critical;
      break;
    case level_enum::off:
      // monitor mode
      return;
    default:
      kLogLevel = currentLvl;
      break;
    }
    spdlog::set_level(kLogLevel);
    spdlog::flush_on(kLogLevel);
  }

private:
  static void initConsoleLogger(LogLevel logLvl) {
    auto consoleSink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    kMainLogger = std::make_shared<spdlog::logger>("console", consoleSink);
    kMainLogger->set_pattern("[%H:%M:%S.%e] [%^%L%$] %v");
    spdlog::set_default_logger(kMainLogger);
  }

  static void initAsyncLogger(LogLevel logLvl, const std::string &filename) {
    spdlog::init_thread_pool(8192, 1);
    kMainLogger = std::make_shared<spdlog::async_logger>(
        "async_file_logger", std::make_shared<spdlog::sinks::basic_file_sink_mt>(filename),
        spdlog::thread_pool());
    kMainLogger->set_pattern("[%H:%M:%S.%e] [%^%L%$] %v");
    spdlog::set_default_logger(kMainLogger);
  };

  static void initRotatingLogger(LogLevel logLvl, const std::string &filename) {
    kMainLogger = spdlog::rotating_logger_mt("rotating_logger", filename, 10485760, 3);
    kMainLogger->set_pattern("[%H:%M:%S.%e] [%^%L%$] %v");
    spdlog::set_default_logger(kMainLogger);
  }

private:
  static void initServiceLogger() {
    auto consoleSink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    kMonitorLogger = std::make_shared<spdlog::logger>("console_monitor", consoleSink);
    kMonitorLogger->set_pattern("[%H:%M:%S.%e] [%^%L%$] %v");
  }

  static SPtrSpdLogger kMainLogger;
  static SPtrSpdLogger kMonitorLogger;
  static LogLevel kLogLevel;
};

LoggerManager::SPtrSpdLogger LoggerManager::kMainLogger{};
LoggerManager::SPtrSpdLogger LoggerManager::kMonitorLogger{};
LoggerManager::LogLevel LoggerManager::kLogLevel{};

} // namespace hft

#endif // HFT_COMMON_