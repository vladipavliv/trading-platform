/**
 * @author Vladimir Pavliv
 * @date 2025-02-13
 */

#ifndef HFT_COMMON_CONSTANTS_HPP
#define HFT_COMMON_CONSTANTS_HPP

namespace hft {

constexpr size_t BUSY_WAIT_CYCLES = 4000;
constexpr size_t BUFFER_SIZE = 1024 * 8;
constexpr size_t ORDER_BOOK_LIMIT = 10000;
constexpr size_t CACHE_LINE_SIZE = 64;
constexpr size_t LOG_FILE_SIZE = 25 * 1024 * 1024;
constexpr size_t MAX_CONNECTIONS = 1000;
constexpr size_t PRICE_FLUCTUATION_RATE = 5;

} // namespace hft

#endif // HFT_COMMON_TYPES_HPP