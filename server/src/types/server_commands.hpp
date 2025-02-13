/**
 * @file
 * @brief
 *
 * @author Vladimir Pavliv
 * @date 2025-02-13
 */

#ifndef HFT_SERVER_COMMAND_HPP
#define HFT_SERVER_COMMAND_HPP

#include <stdint.h>

namespace hft::server {

enum class ServerCommand : uint8_t {
  MarketFeedStart = 0U,
  MarketFeedStop = 1U,
  ShowStats = 2U,
  Shutdown = 3U
};

} // namespace hft::server

#endif // HFT_SERVER_DISPATCHER_COMMAND_HPP