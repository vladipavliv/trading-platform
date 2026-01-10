/**
 * @author Vladimir Pavliv
 * @date 2025-08-04
 */

#include <iostream>
#include <thread>

#include <gtest/gtest.h>

#include "ptr_types.hpp"
#include "sloth_buffer.hpp"
#include "utils/test_utils.hpp"

namespace hft::tests {

using namespace utils;

TEST(SlothBufferTest, Basic) {
  UPtr<SlothBuffer> buf = std::make_unique<SlothBuffer>();

  std::vector<Order> orders;
  orders.reserve(1000);

  const size_t orderSize = sizeof(Order);

  for (int i = 0; i < 1000; ++i) {
    orders.push_back(generateOrder());
  }

  uint64_t watchdog = 0;
  std::jthread T{[&]() {
    std::vector<Order>::iterator iter = orders.begin();
    while (iter != orders.end()) {
      Order o;
      auto readSize = buf->read(reinterpret_cast<uint8_t *>(&o), orderSize);
      if (readSize == 0) {
        if (++watchdog > 100'000) {
          FAIL() << "Timeout: Consumer stuck waiting for data";
        }
        asm volatile("pause" ::: "memory");
        continue;
      }
      ASSERT_TRUE(readSize == orderSize);
      ASSERT_TRUE(o == *iter);
      iter++;
    }
  }};

  for (auto &o : orders) {
    buf->write(reinterpret_cast<const uint8_t *>(&o), orderSize);
  }
}

} // namespace hft::tests
