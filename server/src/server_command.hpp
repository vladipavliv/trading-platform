/**
 * @author Vladimir Pavliv
 * @date 2025-03-06
 */

#ifndef HFT_SERVER_SERVERCOMMAND_HPP
#define HFT_SERVER_SERVERCOMMAND_HPP

#include <stdint.h>

namespace hft::server {

enum class ServerCommand : uint8_t { PriceFeedStart, PriceFeedStop, Shutdown };

}

#endif // HFT_SERVER_SERVERCOMMAND_HPP