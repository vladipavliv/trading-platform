/**
 * @author Vladimir Pavliv
 * @date 2025-03-06
 */

#ifndef HFT_SERVER_TRADERCOMMAND_HPP
#define HFT_SERVER_TRADERCOMMAND_HPP

#include <stdint.h>

namespace hft::trader {

enum class TraderCommand : uint8_t {
  TradeStart,
  TradeStop,
  TradeSpeedUp,
  TradeSpeedDown,
  Shutdown
};

}

#endif // HFT_SERVER_TRADERCOMMAND_HPP