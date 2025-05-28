/**
 * @author Vladimir Pavliv
 * @date 2025-04-24
 */

#ifndef HFT_TEST_CONSUMERSPY_HPP
#define HFT_TEST_CONSUMERSPY_HPP

#include <any>
#include <typeinfo>

#include <gtest/gtest.h>

#include "types.hpp"

namespace hft::tests {

/**
 * @brief Spy for a message consumer
 */
struct ConsumerSpy {
  template <typename EventType>
  void post(CRef<EventType> event) {
    data.push_back(event);
  }

  template <typename EventType>
  bool checkType(size_t index) {
    if (index >= data.size()) {
      return false;
    }
    return data[index].type() == typeid(EventType);
  }

  template <typename EventType>
  bool checkValue(size_t index, CRef<EventType> value) {
    if (!checkType<EventType>(index)) {
      return false;
    }
    return std::any_cast<EventType>(data[index]) == value;
  }

  std::vector<std::any> data;
};

} // namespace hft::tests

#endif // HFT_TEST_CONSUMERSPY_HPP