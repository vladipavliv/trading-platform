/**
 * @author Vladimir Pavliv
 * @date 2025-08-04
 */

#include <iostream>

#include <gtest/gtest.h>

#include "bus/data_bus.hpp"
#include "config/server_config.hpp"
#include "execution/order_book.hpp"

namespace hft::tests {

using namespace server;
using namespace utils;

class DataBusFixture : public ::testing::Test {
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

TEST_F(DataBusFixture, Post) {
  DataBus<size_t> bus;

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

  bus.stop();
  ASSERT_EQ(eventsPushed, eventsPopped.load());
}

} // namespace hft::tests
