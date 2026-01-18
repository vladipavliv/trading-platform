/**
 * @author Vladimir Pavliv
 * @date 2026-01-08
 */

#ifndef HFT_COMMON_IORESULT_HPP
#define HFT_COMMON_IORESULT_HPP

#include <cstdint>

namespace hft {
enum class IoStatus : uint8_t { Ok, WouldBlock, Closed, Error };

struct IoResult {
  uint32_t bytes;
  IoStatus code;

  constexpr explicit operator bool() const noexcept { return code == IoStatus::Ok; }
};

inline String toString(const IoStatus &msg) {
  switch (msg) {
  case IoStatus::Ok:
    return "Ok";
  case IoStatus::WouldBlock:
    return "WouldBlock";
  case IoStatus::Closed:
    return "Closed";
  case IoStatus::Error:
    return "Error";
  default:
    return "Unknown";
  }
}

} // namespace hft

#endif // HFT_COMMON_IORESULT_HPP