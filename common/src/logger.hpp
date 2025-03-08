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

#include "constants.hpp"
#include "utils/string_utils.hpp"

namespace hft {

/**
 * @brief Logger wrapper with a separate spd logger for main logs and monitor logs
 * main logs are written to a file asynchronously, monitor logs are written to a console
 * @details for a non-trivial log messages like toString(OrderStatus) or typeid(EventType).name()
 * logging is done via lambda so that arguments are not evaluated unnecesserily
 * spdlog::debug([](){ return typeid(EventType).name(); }());
 */
class Logger {
public:
  static constexpr auto LOG_PATTERN = "%H:%M:%S.%f [%^%L%$] %v";

  using LogLevel = spdlog::level::level_enum;
  using SPtrSpdLogger = std::shared_ptr<spdlog::logger>;

  static SPtrSpdLogger mainLogger;
  static SPtrSpdLogger monitorLogger;

  static void initialize(LogLevel logLevel, const std::string &fileName = "") {
    initMonitorLogger();
    initMainLogger(fileName);
    spdlog::set_level(logLevel);
    spdlog::flush_on(spdlog::level::err);
  }

  static void switchLogLevel(bool goUp) {
    using namespace spdlog::level;
    level_enum currentLevel = spdlog::get_level();
    switch (currentLevel) {
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
    default:
      break;
    }
  }

private:
  static void initMonitorLogger() {
    auto consoleSink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    monitorLogger = std::make_shared<spdlog::logger>("console_monitor", consoleSink);
    monitorLogger->set_pattern(LOG_PATTERN);
  }

  static void initMainLogger(const std::string &filename) {
    spdlog::init_thread_pool(8192, 1);
    auto rotatingSink =
        std::make_shared<spdlog::sinks::rotating_file_sink_mt>(filename, LOG_FILE_SIZE, 3);
    mainLogger = std::make_shared<spdlog::async_logger>(
        "async_file_logger", rotatingSink, spdlog::thread_pool(),
        spdlog::async_overflow_policy::overrun_oldest);
    mainLogger->set_pattern(LOG_PATTERN);
    spdlog::register_logger(mainLogger);
    spdlog::set_default_logger(mainLogger);
  }
};

Logger::SPtrSpdLogger Logger::mainLogger{};
Logger::SPtrSpdLogger Logger::monitorLogger{};

} // namespace hft

#endif // HFT_COMMON_LOGGER_HPP
