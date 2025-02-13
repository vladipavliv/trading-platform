/**
 * @file
 * @brief
 *
 * @author Vladimir Pavliv
 * @date 2025-02-13
 */

#ifndef HFT_COMMON_TYPES_HPP
#define HFT_COMMON_TYPES_HPP

#include <array>
#include <cstddef>
#include <cstdint>
#include <stdint.h>
#include <string>
#include <vector>

namespace hft {

using String = std::string;
using StringView = std::string_view;
using ObjectId = uintptr_t;

constexpr size_t BUFFER_SIZE = 1024;
using Buffer = std::array<uint8_t, BUFFER_SIZE>;

using CpuId = uint8_t;

} // namespace hft

#endif // HFT_COMMON_TYPES_HPP