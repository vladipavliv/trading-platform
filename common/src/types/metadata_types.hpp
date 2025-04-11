/**
 * @author Vladimir Pavliv
 * @date 2025-04-08
 */

#ifndef HFT_COMMON_MARKETMETADATATYPES_HPP
#define HFT_COMMON_MARKETMETADATATYPES_HPP

#include "market_types.hpp"
#include "types.hpp"

namespace hft {

enum class TimestampType : uint8_t { Created, Received, Fulfilled, Notified };

struct OrderTimestamp {
  const OrderId orderId;
  const Timestamp timestamp;
  const TimestampType type;
};

} // namespace hft

#endif // HFT_COMMON_MARKETMETADATATYPES_HPP
