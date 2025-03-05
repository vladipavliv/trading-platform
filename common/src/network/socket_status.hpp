/**
 * @author Vladimir Pavliv
 * @date 2025-02-13
 */

#ifndef HFT_COMMON_SOCKETSTATUS_HPP
#define HFT_COMMON_SOCKETSTATUS_HPP

#include <stdint.h>

#include "market_types.hpp"
#include "types.hpp"

namespace hft {

enum class SocketStatus : uint8_t { Connected = 0U, Disconnected = 1U };

struct SocketStatusEvent {
  TraderId traderId;
  SocketStatus status;
};

} // namespace hft

#endif // HFT_COMMON_SOCKETSTATUS_HPP