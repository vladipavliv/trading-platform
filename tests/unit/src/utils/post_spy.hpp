/**
 * @author Vladimir Pavliv
 * @date 2025-04-24
 */

#ifndef HFT_TEST_POSTSPY_HPP
#define HFT_TEST_POSTSPY_HPP

#include <any>
#include <typeinfo>

#include <gtest/gtest.h>

#include "domain/server_auth_messages.hpp"
#include "functional_types.hpp"
#include "logging.hpp"
#include "primitive_types.hpp"
#include "utils/string_utils.hpp"

namespace hft::tests {

/**
 * @brief Spy for a message consumer
 */
struct PostSpy {
  template <typename EventType>
  void post(CRef<EventType> event) {
    data.push_back(event);
    printers.push_back(
        [event]() { std::cout << typeid(event).name() << " " << toString(event) << std::endl; });
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

  void printAll() {
    std::cout << "PostSpy " << data.size() << " messages" << std::endl;
    for (auto &printer : printers) {
      printer();
    }
  }

  void print(size_t idx) {
    assert(idx < data.size());
    printers[idx]();
  }

  size_t size() const { return data.size(); }

  std::vector<std::any> data;
  std::vector<StdCallback> printers;
};

} // namespace hft::tests

#endif // HFT_TEST_POSTSPY_HPP