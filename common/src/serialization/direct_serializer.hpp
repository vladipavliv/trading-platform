/**
 * @author Vladimir Pavliv
 * @date 2025-02-13
 */

#ifndef HFT_COMMON_SERIALIZATION_DIRECTSERIALIZER_HPP
#define HFT_COMMON_SERIALIZATION_DIRECTSERIALIZER_HPP

#include "types/market_types.hpp"
#include "types/types.hpp"

namespace hft::serialization {

/**
 * @brief Direct memory copy
 */
class DirectSerializer {
public:
  template <typename MessageType>
  static MessageType deserialize(const uint8_t *buffer) {
    MessageType result;
    memcpy(&result, buffer, sizeof(result));
    return result;
  }

  template <typename MessageType>
  static void serialize(const MessageType &order, uint8_t *buffer) {
    memcpy(buffer, &order, sizeof(order));
  }
};

} // namespace hft::serialization

#endif // HFT_COMMON_SERIALIZATION_DIRECTSERIALIZER_HPP
