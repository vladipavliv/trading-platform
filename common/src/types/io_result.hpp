/**
 * @author Vladimir Pavliv
 * @date 2026-01-08
 */

#ifndef HFT_COMMON_IORESULT_HPP
#define HFT_COMMON_IORESULT_HPP

#include <cstdint>

namespace hft {
enum class IoResult : uint8_t { Ok, WouldBlock, Closed, Error };
}

#endif // HFT_COMMON_IORESULT_HPP