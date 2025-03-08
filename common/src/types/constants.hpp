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
constexpr size_t LFQ_POP_LIMIT = 20;
constexpr size_t BUSY_WAIT_CYCLES = 1000000;
constexpr size_t ORDER_BOOK_LIMIT = 5000;
constexpr size_t CACHE_LINE_SIZE = 64;
constexpr size_t MAX_SERIALIZED_MESSAGE_SIZE = 64;
constexpr size_t PRICE_UPDATE_CHUNK = 5;
constexpr size_t LOG_FILE_SIZE = 5 * 1024 * 1024;

constexpr float PRICE_FLUCTUATION_RATE = 5; // %

} // namespace hft

#endif // HFT_COMMON_TYPES_HPP