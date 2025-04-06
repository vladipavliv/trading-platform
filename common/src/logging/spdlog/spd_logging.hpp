/**
 * @author Vladimir Pavliv
 * @date 2025-04-06
 */

#ifndef HFT_COMMON_SPDLOGGING_HPP
#define HFT_COMMON_SPDLOGGING_HPP

#include "spd_logger.hpp"

#define FILE_NAME (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)

#define LOG_INIT(filename) hft::SpdLogger::initialize(filename);

#if SPDLOG_ACTIVE_LEVEL <= SPDLOG_LEVEL_DEBUG
#define LOG_DEBUG(msg, ...)                                                                        \
  spdlog::debug("[{} {}:{}] " msg, FILE_NAME, __FUNCTION__, __LINE__, ##__VA_ARGS__);
#else
#define LOG_DEBUG(msg, ...) (void)0
#endif

#if SPDLOG_ACTIVE_LEVEL <= SPDLOG_LEVEL_INFO
#define LOG_INFO(msg, ...)                                                                         \
  spdlog::info("[{} {}:{}] " msg, FILE_NAME, __FUNCTION__, __LINE__, ##__VA_ARGS__);
#else
#define LOG_INFO(msg, ...) (void)0
#endif

#if SPDLOG_ACTIVE_LEVEL <= SPDLOG_LEVEL_WARN
#define LOG_WARN(msg, ...)                                                                         \
  spdlog::warn("[{} {}:{}] " msg, FILE_NAME, __FUNCTION__, __LINE__, ##__VA_ARGS__);
#else
#define LOG_WARN(msg, ...) (void)0
#endif

#if SPDLOG_ACTIVE_LEVEL <= SPDLOG_LEVEL_ERROR
#define LOG_ERROR(msg, ...)                                                                        \
  spdlog::error("[{} {}:{}] " msg, FILE_NAME, __FUNCTION__, __LINE__, ##__VA_ARGS__);
#else
#define LOG_ERROR(msg, ...) (void)0
#endif

#if SPDLOG_ACTIVE_LEVEL <= SPDLOG_LEVEL_DEBUG
#define LOG_DEBUG_SYSTEM(msg, ...)                                                                 \
  {                                                                                                \
    LOG_DEBUG(msg, ##__VA_ARGS__);                                                                 \
    if (hft::SpdLogger::consoleLogger) {                                                           \
      hft::SpdLogger::consoleLogger->debug(msg, ##__VA_ARGS__);                                    \
    }                                                                                              \
  }
#else
#define LOG_DEBUG_SYSTEM(msg, ...) (void)0
#endif

#if SPDLOG_ACTIVE_LEVEL <= SPDLOG_LEVEL_INFO
#define LOG_INFO_SYSTEM(msg, ...)                                                                  \
  {                                                                                                \
    LOG_INFO(msg, ##__VA_ARGS__);                                                                  \
    if (hft::SpdLogger::consoleLogger) {                                                           \
      hft::SpdLogger::consoleLogger->info(msg, ##__VA_ARGS__);                                     \
    }                                                                                              \
  }
#else
#define LOG_INFO_SYSTEM(msg, ...) (void)0
#endif

#if SPDLOG_ACTIVE_LEVEL <= SPDLOG_LEVEL_WARN
#define LOG_WARN_SYSTEM(msg, ...)                                                                  \
  {                                                                                                \
    LOG_WARN(msg, ##__VA_ARGS__);                                                                  \
    if (hft::SpdLogger::consoleLogger) {                                                           \
      hft::SpdLogger::consoleLogger->warn(msg, ##__VA_ARGS__);                                     \
    }                                                                                              \
  }
#else
#define LOG_WARN_SYSTEM(msg, ...) (void)0
#endif

#if SPDLOG_ACTIVE_LEVEL <= SPDLOG_LEVEL_ERROR
#define LOG_ERROR_SYSTEM(msg, ...)                                                                 \
  {                                                                                                \
    LOG_ERROR(msg, ##__VA_ARGS__);                                                                 \
    if (hft::SpdLogger::consoleLogger) {                                                           \
      hft::SpdLogger::consoleLogger->error(msg, ##__VA_ARGS__);                                    \
    }                                                                                              \
  }
#else
#define LOG_ERROR_SYSTEM(msg, ...) (void)0
#endif

#endif // HFT_COMMON_SPDLOGGING_HPP
