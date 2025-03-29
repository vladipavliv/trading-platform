/**
 * @author Vladimir Pavliv
 * @date 2025-02-13
 */

#ifndef HFT_COMMON_CONNECTIONSTATUS_HPP
#define HFT_COMMON_CONNECTIONSTATUS_HPP

#include <stdint.h>

#include "market_types.hpp"
#include "types.hpp"

namespace hft {

enum class ConnectionStatus : uint8_t { Connected, Disconnected, Error };

struct TcpConnectionStatus {
  TraderId traderId;
  ConnectionStatus status;
};

struct UdpConnectionStatus {
  ConnectionStatus status;
};

} // namespace hft

#endif // HFT_COMMON_CONNECTIONSTATUS_HPP