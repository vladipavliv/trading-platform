/**
 * @author Vladimir Pavliv
 * @date 2026-01-07
 */

#ifndef HFT_COMMON_CONTAINERTYPES_HPP
#define HFT_COMMON_CONTAINERTYPES_HPP

#include <cstdint>
#include <span>
#include <vector>

namespace hft {

template <typename T>
using Vector = std::vector<T>;

template <typename T>
using Span = std::span<T>;

using ByteSpan = Span<uint8_t>;
using ByteBuffer = std::vector<uint8_t>;

} // namespace hft

#endif // HFT_COMMON_CONTAINERTYPES_HPP
