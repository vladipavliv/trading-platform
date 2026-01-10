/**
 * @author Vladimir Pavliv
 * @date 2025-08-04
 */

#include <iostream>
#include <thread>

#include <gtest/gtest.h>

#include "slot_buffer.hpp"
#include "utils/test_utils.hpp"

namespace hft::tests {

using namespace utils;

TEST(SlotBufferTest, Basic) {
  SlotBuffer buf;
  AtomicBool run{true};
  std::vector<Order> orders;
  orders.reserve(1000);

  for (int i = 0; i < 1000; ++i) {
    orders.push_back(generateOrder());
  }

  std::jthread T{[&]() {
    std::vector<Order>::iterator iter = orders.begin();
    while (true) {
      Order o;
      while (buf.read(reinterpret_cast<uint8_t *>(&o), sizeof(o))) {
        ASSERT_TRUE(o == *iter);
        iter++;
        if (iter == orders.end()) {
          return;
        }
      }
      std::this_thread::yield();
    }
  }};

  for (auto &o : orders) {
    buf.write(reinterpret_cast<const uint8_t *>(&o), sizeof(o));
  }
}

} // namespace hft::tests
