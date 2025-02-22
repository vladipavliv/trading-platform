/**
 * @author Vladimir Pavliv
 * @date 2025-02-13
 */

#ifndef HFT_COMMON_CONSTANTS_HPP
#define HFT_COMMON_CONSTANTS_HPP

#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <span>
#include <stdint.h>
#include <string>
#include <vector>

namespace hft {

constexpr size_t BUFFER_SIZE = 1024 * 8;
constexpr size_t LFQ_SIZE = 1024;
constexpr size_t LFQ_POP_LIMIT = 10;
constexpr size_t BUSY_WAIT_CYCLES = 1000000;
constexpr size_t ORDER_BOOK_LIMIT = 1000;
constexpr size_t CACHE_LINE_SIZE = 64;

} // namespace hft

#endif // HFT_COMMON_TYPES_HPP