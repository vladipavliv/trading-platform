/**
 * @author Vladimir Pavliv
 * @date 2025-04-06
 */

#ifndef HFT_COMMON_SPDLOGGING_HPP
#define HFT_COMMON_SPDLOGGING_HPP

#include <iostream>

#include "spd_logger.hpp"

namespace hft {

#define FILE_NAME (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)

#define LOG_INIT(filename) SpdLogger::initialize(filename);

#if SPDLOG_ACTIVE_LEVEL <= SPDLOG_LEVEL_TRACE
#define LOG_TRACE(msg, ...)                                                                        \
  do {                                                                                             \
    if (SpdLogger::fileLogger) {                                                                   \
      SpdLogger::fileLogger->trace("[{} {}:{}] " msg, FILE_NAME, __FUNCTION__, __LINE__,           \
                                   ##__VA_ARGS__);                                                 \
    }                                                                                              \
  } while (0)
#else
#define LOG_TRACE(msg, ...) (void)0
#endif

#if SPDLOG_ACTIVE_LEVEL <= SPDLOG_LEVEL_DEBUG
#define LOG_DEBUG(msg, ...)                                                                        \
  do {                                                                                             \
    if (SpdLogger::fileLogger) {                                                                   \
      SpdLogger::fileLogger->debug("[{} {}:{}] " msg, FILE_NAME, __FUNCTION__, __LINE__,           \
                                   ##__VA_ARGS__);                                                 \
    }                                                                                              \
  } while (0)
#else
#define LOG_DEBUG(msg, ...) (void)0
#endif

#if SPDLOG_ACTIVE_LEVEL <= SPDLOG_LEVEL_INFO
#define LOG_INFO(msg, ...)                                                                         \
  do {                                                                                             \
    if (SpdLogger::fileLogger) {                                                                   \
      SpdLogger::fileLogger->info("[{} {}:{}] " msg, FILE_NAME, __FUNCTION__, __LINE__,            \
                                  ##__VA_ARGS__);                                                  \
    }                                                                                              \
  } while (0)
#else
#define LOG_INFO(msg, ...) (void)0
#endif

#if SPDLOG_ACTIVE_LEVEL <= SPDLOG_LEVEL_WARN
#define LOG_WARN(msg, ...)                                                                         \
  do {                                                                                             \
    if (SpdLogger::fileLogger) {                                                                   \
      SpdLogger::fileLogger->warn("[{} {}:{}] " msg, FILE_NAME, __FUNCTION__, __LINE__,            \
                                  ##__VA_ARGS__);                                                  \
    }                                                                                              \
  } while (0)
#else
#define LOG_WARN(msg, ...) (void)0
#endif

#if SPDLOG_ACTIVE_LEVEL <= SPDLOG_LEVEL_ERROR
#define LOG_ERROR(msg, ...)                                                                        \
  do {                                                                                             \
    if (SpdLogger::fileLogger) {                                                                   \
      SpdLogger::fileLogger->error("[{} {}:{}] " msg, FILE_NAME, __FUNCTION__, __LINE__,           \
                                   ##__VA_ARGS__);                                                 \
    }                                                                                              \
  } while (0)
#else
#define LOG_ERROR(msg, ...) (void)0
#endif

#if SPDLOG_ACTIVE_LEVEL <= SPDLOG_LEVEL_CRITICAL
#define LOG_CRITICAL(msg, ...)                                                                     \
  do {                                                                                             \
    if (SpdLogger::fileLogger) {                                                                   \
      SpdLogger::fileLogger->critical("[{} {}:{}] " msg, FILE_NAME, __FUNCTION__, __LINE__,        \
                                      ##__VA_ARGS__);                                              \
    }                                                                                              \
  } while (0)
#else
#define LOG_CRITICAL(msg, ...) (void)0
#endif

#if SPDLOG_ACTIVE_LEVEL <= SPDLOG_LEVEL_TRACE
#define LOG_TRACE_SYSTEM(msg, ...)                                                                 \
  {                                                                                                \
    LOG_TRACE(msg, ##__VA_ARGS__);                                                                 \
    if (SpdLogger::consoleLogger) {                                                                \
      SpdLogger::consoleLogger->trace(msg, ##__VA_ARGS__);                                         \
    }                                                                                              \
  }
#else
#define LOG_TRACE_SYSTEM(msg, ...)                                                                 \
  if (SpdLogger::consoleLogger) {                                                                  \
    SpdLogger::consoleLogger->trace(msg, ##__VA_ARGS__);                                           \
  }
#endif

#if SPDLOG_ACTIVE_LEVEL <= SPDLOG_LEVEL_DEBUG
#define LOG_DEBUG_SYSTEM(msg, ...)                                                                 \
  {                                                                                                \
    LOG_DEBUG(msg, ##__VA_ARGS__);                                                                 \
    if (SpdLogger::consoleLogger) {                                                                \
      SpdLogger::consoleLogger->debug(msg, ##__VA_ARGS__);                                         \
    }                                                                                              \
  }
#else
#define LOG_DEBUG_SYSTEM(msg, ...)                                                                 \
  if (SpdLogger::consoleLogger) {                                                                  \
    SpdLogger::consoleLogger->debug(msg, ##__VA_ARGS__);                                           \
  }
#endif

#if SPDLOG_ACTIVE_LEVEL <= SPDLOG_LEVEL_INFO
#define LOG_INFO_SYSTEM(msg, ...)                                                                  \
  {                                                                                                \
    LOG_INFO(msg, ##__VA_ARGS__);                                                                  \
    if (SpdLogger::consoleLogger) {                                                                \
      SpdLogger::consoleLogger->info(msg, ##__VA_ARGS__);                                          \
    }                                                                                              \
  }
#else
#define LOG_INFO_SYSTEM(msg, ...)                                                                  \
  if (SpdLogger::consoleLogger) {                                                                  \
    SpdLogger::consoleLogger->info(msg, ##__VA_ARGS__);                                            \
  }
#endif

#if SPDLOG_ACTIVE_LEVEL <= SPDLOG_LEVEL_WARN
#define LOG_WARN_SYSTEM(msg, ...)                                                                  \
  {                                                                                                \
    LOG_WARN(msg, ##__VA_ARGS__);                                                                  \
    if (SpdLogger::consoleLogger) {                                                                \
      SpdLogger::consoleLogger->warn(msg, ##__VA_ARGS__);                                          \
    }                                                                                              \
  }
#else
#define LOG_WARN_SYSTEM(msg, ...)                                                                  \
  if (SpdLogger::consoleLogger) {                                                                  \
    SpdLogger::consoleLogger->warn(msg, ##__VA_ARGS__);                                            \
  }
#endif

#if SPDLOG_ACTIVE_LEVEL <= SPDLOG_LEVEL_ERROR
#define LOG_ERROR_SYSTEM(msg, ...)                                                                 \
  {                                                                                                \
    LOG_ERROR(msg, ##__VA_ARGS__);                                                                 \
    if (SpdLogger::consoleLogger) {                                                                \
      SpdLogger::consoleLogger->error(msg, ##__VA_ARGS__);                                         \
    }                                                                                              \
  }
#else
#define LOG_ERROR_SYSTEM(msg, ...)                                                                 \
  if (SpdLogger::consoleLogger) {                                                                  \
    SpdLogger::consoleLogger->error(msg, ##__VA_ARGS__);                                           \
  }
#endif

#if SPDLOG_ACTIVE_LEVEL <= SPDLOG_LEVEL_CRITICAL
#define LOG_CRITICAL_SYSTEM(msg, ...)                                                              \
  {                                                                                                \
    LOG_CRITICAL(msg, ##__VA_ARGS__);                                                              \
    if (SpdLogger::consoleLogger) {                                                                \
      SpdLogger::consoleLogger->critical(msg, ##__VA_ARGS__);                                      \
    }                                                                                              \
  }
#else
#define LOG_CRITICAL_SYSTEM(msg, ...)                                                              \
  if (SpdLogger::consoleLogger) {                                                                  \
    SpdLogger::consoleLogger->critical(msg, ##__VA_ARGS__);                                        \
  }
#endif

} // namespace hft

#endif // HFT_COMMON_SPDLOGGING_HPP
