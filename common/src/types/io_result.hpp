/**
 * @author Vladimir Pavliv
 * @date 2026-01-08
 */

#ifndef HFT_COMMON_IORESULT_HPP
#define HFT_COMMON_IORESULT_HPP

#include <cstdint>

namespace hft {
enum class IoResult : uint8_t { Ok, WouldBlock, Closed, Error };

inline String toString(const IoResult &msg) {
  switch (msg) {
  case IoResult::Ok:
    return "Ok";
  case IoResult::WouldBlock:
    return "WouldBlock";
  case IoResult::Closed:
    return "Closed";
  case IoResult::Error:
    return "Error";
  default:
    return "Unknown";
  }
}

} // namespace hft

#endif // HFT_COMMON_IORESULT_HPP