/**
 * @author Vladimir Pavliv
 * @date 2025-04-06
 */

#ifndef HFT_COMMON_SPDLOGGER_HPP
#define HFT_COMMON_SPDLOGGER_HPP

#include <memory>

#include <spdlog/async.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

#include "constants.hpp"

namespace hft {

/**
 * @brief Logger with two static spdlog instances:
 * - main logger to log into a file
 * - system logger to log into a console
 */
class SpdLogger {
  static constexpr auto LOG_PATTERN = "%H:%M:%S.%f [%^%L%$] %v";

public:
  using LogLevel = spdlog::level::level_enum;
  using SPtrSpdLogger = std::shared_ptr<spdlog::logger>;

  static SPtrSpdLogger consoleLogger;
  static SPtrSpdLogger fileLogger;

  static void initialize(const std::string &fileName = "") {
    if (consoleLogger != nullptr || fileLogger != nullptr) {
      return;
    }
    const LogLevel logLevel = static_cast<LogLevel>(SPDLOG_ACTIVE_LEVEL);

    initConsoleLogger();
    initFileLogger(fileName);

    spdlog::set_level(logLevel);
    spdlog::flush_on(spdlog::level::err);
  }

private:
  static void initConsoleLogger() {
    auto consoleSink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    consoleLogger = std::make_shared<spdlog::logger>("console_monitor", consoleSink);
    consoleLogger->set_pattern(LOG_PATTERN);
  }

  static void initFileLogger(const std::string &filename) {
    spdlog::init_thread_pool(8192, 1);
    auto rotatingSink =
        std::make_shared<spdlog::sinks::rotating_file_sink_mt>(filename, LOG_FILE_SIZE, 3);
    fileLogger = std::make_shared<spdlog::async_logger>(
        "async_file_logger", rotatingSink, spdlog::thread_pool(),
        spdlog::async_overflow_policy::overrun_oldest);
    fileLogger->set_pattern(LOG_PATTERN);
    spdlog::register_logger(fileLogger);
    spdlog::set_default_logger(fileLogger);
  }
};

} // namespace hft

#endif // HFT_COMMON_SPDLOGGER_HPP
