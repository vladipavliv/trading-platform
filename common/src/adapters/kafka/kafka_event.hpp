/**
 * @author Vladimir Pavliv
 * @date 2025-04-19
 */

#ifndef HFT_COMMON_ADAPTERS_KAFKAEVENT_HPP
#define HFT_COMMON_ADAPTERS_KAFKAEVENT_HPP

#include <stdint.h>

#include "utils/string_utils.hpp"

namespace hft {
namespace adapters {
enum class KafkaStatus : uint8_t { Error };

struct KafkaEvent {
  KafkaStatus status;
  String details;
};
} // namespace adapters

inline String toString(const adapters::KafkaStatus &status) {
  switch (status) {
  case adapters::KafkaStatus::Error:
    return "error";
  default:
    return std::format("unknown kafka status {}", static_cast<uint8_t>(status));
  }
}
inline String toString(const adapters::KafkaEvent &event) {
  return std::format("KafkaEvent {} {}", toString(event.status), event.details);
}

} // namespace hft

#endif // HFT_COMMON_ADAPTERS_KAFKAEVENT_HPP