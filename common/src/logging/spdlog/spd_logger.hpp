/**
 * @author Vladimir Pavliv
 * @date 2026-01-04
 */

#ifndef HFT_COMMON_SPDLOGGER_HPP
#define HFT_COMMON_SPDLOGGER_HPP

#include <memory>
#include <spdlog/spdlog.h>
#include <string>

#include "config/config.hpp"

namespace hft {

/**
 * @brief Logger with two static spdlog instances:
 * - main logger to log into a file
 * - system logger to log into a console
 */
class SpdLogger {
  static constexpr auto LOG_PATTERN = "%H:%M:%S.%f [%^%L%$] %v";

public:
  using SPtrSpdLogger = std::shared_ptr<spdlog::logger>;

  static SPtrSpdLogger consoleLogger;
  static SPtrSpdLogger fileLogger;

  static void initialize(const Config &cfg);

  static constexpr const char *simpleFileName(const char *path) {
    const char *file = path;
    while (*path) {
      if (*path == '/')
        file = path + 1;
      path++;
    }
    return file;
  }
};

#define LOG_INIT(cfg) SpdLogger::initialize(cfg);

#define LOG_BASE(level, msg, ...)                                                                  \
  do {                                                                                             \
    auto *logger_ptr = spdlog::default_logger_raw();                                               \
    if (logger_ptr != nullptr) [[likely]] {                                                        \
      logger_ptr->log(                                                                             \
          spdlog::source_loc{hft::SpdLogger::simpleFileName(__FILE__), __LINE__, __FUNCTION__},    \
          level, msg, ##__VA_ARGS__);                                                              \
    }                                                                                              \
  } while (0)

// --- TRACE ---
#if SPDLOG_ACTIVE_LEVEL <= SPDLOG_LEVEL_TRACE
#define LOG_TRACE(msg, ...) LOG_BASE(spdlog::level::trace, msg, ##__VA_ARGS__)
#else
#define LOG_TRACE(msg, ...) (void)0
#endif

// --- DEBUG ---
#if SPDLOG_ACTIVE_LEVEL <= SPDLOG_LEVEL_DEBUG
#define LOG_DEBUG(msg, ...) LOG_BASE(spdlog::level::debug, msg, ##__VA_ARGS__)
#else
#define LOG_DEBUG(msg, ...) (void)0
#endif

// --- INFO ---
#if SPDLOG_ACTIVE_LEVEL <= SPDLOG_LEVEL_INFO
#define LOG_INFO(msg, ...) LOG_BASE(spdlog::level::info, msg, ##__VA_ARGS__)
#else
#define LOG_INFO(msg, ...) (void)0
#endif

// --- WARN ---
#if SPDLOG_ACTIVE_LEVEL <= SPDLOG_LEVEL_WARN
#define LOG_WARN(msg, ...) LOG_BASE(spdlog::level::warn, msg, ##__VA_ARGS__)
#else
#define LOG_WARN(msg, ...) (void)0
#endif

// --- ERROR ---
#if SPDLOG_ACTIVE_LEVEL <= SPDLOG_LEVEL_ERROR
#define LOG_ERROR(msg, ...) LOG_BASE(spdlog::level::err, msg, ##__VA_ARGS__)
#else
#define LOG_ERROR(msg, ...) (void)0
#endif

// --- TRACE SYSTEM ---
#define LOG_TRACE_SYSTEM(msg, ...)                                                                 \
  do {                                                                                             \
    LOG_TRACE(msg, ##__VA_ARGS__);                                                                 \
    if (hft::SpdLogger::consoleLogger)                                                             \
      hft::SpdLogger::consoleLogger->trace(msg, ##__VA_ARGS__);                                    \
  } while (0)

// --- DEBUG SYSTEM ---
#define LOG_DEBUG_SYSTEM(msg, ...)                                                                 \
  do {                                                                                             \
    LOG_DEBUG(msg, ##__VA_ARGS__);                                                                 \
    if (hft::SpdLogger::consoleLogger)                                                             \
      hft::SpdLogger::consoleLogger->debug(msg, ##__VA_ARGS__);                                    \
  } while (0)

// --- INFO SYSTEM ---
#define LOG_INFO_SYSTEM(msg, ...)                                                                  \
  do {                                                                                             \
    LOG_INFO(msg, ##__VA_ARGS__);                                                                  \
    if (hft::SpdLogger::consoleLogger)                                                             \
      hft::SpdLogger::consoleLogger->info(msg, ##__VA_ARGS__);                                     \
  } while (0)

// --- WARN SYSTEM ---
#define LOG_WARN_SYSTEM(msg, ...)                                                                  \
  do {                                                                                             \
    LOG_WARN(msg, ##__VA_ARGS__);                                                                  \
    if (hft::SpdLogger::consoleLogger)                                                             \
      hft::SpdLogger::consoleLogger->warn(msg, ##__VA_ARGS__);                                     \
  } while (0)

// --- ERROR SYSTEM ---
#define LOG_ERROR_SYSTEM(msg, ...)                                                                 \
  do {                                                                                             \
    LOG_ERROR(msg, ##__VA_ARGS__);                                                                 \
    if (hft::SpdLogger::consoleLogger)                                                             \
      hft::SpdLogger::consoleLogger->error(msg, ##__VA_ARGS__);                                    \
  } while (0)

} // namespace hft

#endif // HFT_COMMON_SPDLOGGER_HPP
