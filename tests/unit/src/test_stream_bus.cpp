/**
 * @author Vladimir Pavliv
 * @date 2025-08-04
 */

#include <iostream>

#include <gtest/gtest.h>

#include "bus/stream_bus.hpp"
#include "config/server_config.hpp"
#include "execution/order_book.hpp"

namespace hft::tests {

using namespace server;
using namespace utils;

class StreamBusFixture : public ::testing::Test {
public:
  inline static std::once_flag initFlag;

  void SetUp() override {
    std::call_once(initFlag, []() {
      // Config could be also loaded in other benches
      ServerConfig::load("utest_server_config.ini");
      LOG_INIT(ServerConfig::cfg.logOutput);
    });
  }

  void TearDown() override {}
};

TEST_F(StreamBusFixture, Post) {
  StreamBus<size_t> bus;

  size_t eventsPushed{0};
  std::atomic_size_t eventsPopped{0};
  const size_t eventsCount{1000000};

  bus.subscribe<size_t>([&eventsPopped](const size_t &event) { eventsPopped.fetch_add(1); });
  bus.run();

  for (size_t idx = 0; idx < eventsCount; ++idx) {
    bus.post(idx);
    ++eventsPushed;
    std::this_thread::yield();
  }
  size_t waitCounter{0};
  while (++waitCounter < 10000 && eventsPushed != eventsPopped.load()) {
    std::this_thread::sleep_for(Microseconds(10));
  }
  bus.stop();
  ASSERT_EQ(eventsPushed, eventsPopped.load());
}

} // namespace hft::tests
