/**
 * @file
 * @brief
 *
 * @author Vladimir Pavliv
 * @date 2025-02-13
 */

#ifndef HFT_COMMON_INITLOGGER_HPP
#define HFT_COMMON_INITLOGGER_HPP

#include <memory>

#include <spdlog/async.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

namespace hft {

void initConsoleLogger() {
  auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
  auto logger = std::make_shared<spdlog::logger>("console", console_sink);

  spdlog::set_default_logger(logger);
  spdlog::set_level(spdlog::level::debug);

  spdlog::set_pattern("[%H:%M:%S.%e] [%^%L%$] %v");

  spdlog::flush_on(spdlog::level::debug);
}

void initAsyncLogger() {
  spdlog::init_thread_pool(8192, 1);
  auto async_file_logger = std::make_shared<spdlog::async_logger>(
      "async_file_logger", std::make_shared<spdlog::sinks::basic_file_sink_mt>("hft_logs.txt"),
      spdlog::thread_pool());

  spdlog::set_default_logger(async_file_logger);
  spdlog::info("Async logging initialized.");
};

void initRotatingLogger() {
  auto rotating_logger = spdlog::rotating_logger_mt("rotating_logger", "hft_logs.txt", 10485760, 3);

  spdlog::set_default_logger(rotating_logger);
  spdlog::info("Rotating logging initialized.");
}

void initLogger() { initConsoleLogger(); }

} // namespace hft

#endif // HFT_COMMON_INITLOGGER_HPP