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
using namespace spdlog;

namespace {
inline level::level_enum fromString(std::string levelStr) {
  std::transform(levelStr.begin(), levelStr.end(), levelStr.begin(),
                 [](unsigned char c) { return std::tolower(c); });

  if (levelStr == "trace") {
    return level::trace;
  }
  if (levelStr == "debug") {
    return level::debug;
  }
  if (levelStr == "info") {
    return level::info;
  }
  if (levelStr == "warn") {
    return level::warn;
  }
  if (levelStr == "error") {
    return level::err;
  }
  if (levelStr == "crit") {
    return level::critical;
  }
  if (levelStr == "off") {
    return level::off;
  }

  return level::info;
}
} // namespace

SpdLogger::SPtrSpdLogger SpdLogger::consoleLogger = nullptr;
SpdLogger::SPtrSpdLogger SpdLogger::fileLogger = nullptr;

void SpdLogger::initialize(const Config &cfg) {
  if (fileLogger) {
    LOG_ERROR_SYSTEM("SpdLogger is already initialized");
    return;
  }

  auto consoleSink = std::make_shared<sinks::stdout_color_sink_mt>();
  consoleLogger = std::make_shared<logger>("console", consoleSink);
  consoleLogger->set_pattern("%H:%M:%S.%f [%^%L%$] %v");

  auto sysLogLvl = fromString(cfg.get<std::string>("log.level"));
  consoleLogger->set_level(sysLogLvl);
  consoleLogger->flush_on(sysLogLvl);

  auto fileName = cfg.get<std::string>("log.output");

  if (fileName.empty()) {
    spdlog::set_default_logger(consoleLogger);
    fileLogger = consoleLogger;
  } else {
    spdlog::init_thread_pool(8192, 1);

    auto rotatingSink = std::make_shared<sinks::rotating_file_sink_mt>(fileName, LOG_FILE_SIZE, 10);

    fileLogger = std::make_shared<async_logger>("async_logger", rotatingSink, spdlog::thread_pool(),
                                                async_overflow_policy::overrun_oldest);

    const auto fileLogLvl = static_cast<level::level_enum>(SPDLOG_ACTIVE_LEVEL);
    fileLogger->set_pattern("%H:%M:%S.%f [%^%L%$] [%s:%#] %v");
    fileLogger->set_level(fileLogLvl);
    fileLogger->flush_on(fileLogLvl);
    spdlog::set_default_logger(fileLogger);
  }
}

} // namespace hft