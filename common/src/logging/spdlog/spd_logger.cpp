/**
 * @author Vladimir Pavliv
 * @date 2026-01-07
 */

#include <spdlog/async.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>

#include "constants.hpp"
#include "logging.hpp"

namespace hft {

SpdLogger::SPtrSpdLogger SpdLogger::consoleLogger = nullptr;
SpdLogger::SPtrSpdLogger SpdLogger::fileLogger = nullptr;

void SpdLogger::initialize(const std::string &fileName) {
  if (fileLogger)
    return;

  auto consoleSink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
  consoleLogger = std::make_shared<spdlog::logger>("console", consoleSink);
  consoleLogger->set_pattern("%H:%M:%S.%f [%^%L%$] %v");
  consoleLogger->set_level(spdlog::level::level_enum::debug);

  if (fileName.empty()) {
    spdlog::set_default_logger(consoleLogger);
    fileLogger = consoleLogger;
  } else {
    spdlog::init_thread_pool(8192, 1);

    auto rotatingSink =
        std::make_shared<spdlog::sinks::rotating_file_sink_mt>(fileName, LOG_FILE_SIZE, 10);

    fileLogger =
        std::make_shared<spdlog::async_logger>("async_logger", rotatingSink, spdlog::thread_pool(),
                                               spdlog::async_overflow_policy::overrun_oldest);

    fileLogger->set_pattern("%H:%M:%S.%f [%^%L%$] [%s:%#] %v");
    fileLogger->set_level(static_cast<spdlog::level::level_enum>(SPDLOG_ACTIVE_LEVEL));
    spdlog::set_default_logger(fileLogger);
  }
}

} // namespace hft