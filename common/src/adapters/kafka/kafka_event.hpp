/**
 * @author Vladimir Pavliv
 * @date 2025-04-19
 */

#ifndef HFT_COMMON_ADAPTERS_KAFKAEVENT_HPP
#define HFT_COMMON_ADAPTERS_KAFKAEVENT_HPP

#include <stdint.h>

#include "utils/string_utils.hpp"

namespace hft {

enum class KafkaStatus : uint8_t { Error };

struct KafkaEvent {
  KafkaStatus status;
  String details;
};

namespace utils {
String toString(const KafkaStatus &status) {
  switch (status) {
  case KafkaStatus::Error:
    return "error";
  default:
    return std::format("unknown kafka status {}", static_cast<uint8_t>(status));
  }
}
String toString(const KafkaEvent &event) {
  return std::format("KafkaEvent {} {}", utils::toString(event.status), event.details);
}
} // namespace utils

} // namespace hft

#endif // HFT_COMMON_ADAPTERS_KAFKAEVENT_HPP