/**
 * @author Vladimir Pavliv
 * @date 2025-02-13
 */

#ifndef HFT_COMMON_CONSTANTS_HPP
#define HFT_COMMON_CONSTANTS_HPP

namespace hft {

constexpr size_t SPIN_RETRIES_HOT = 1000;
constexpr size_t SPIN_RETRIES_WARM = SPIN_RETRIES_HOT * 100;
constexpr size_t SPIN_RETRIES_YIELD = SPIN_RETRIES_WARM * 10;
constexpr size_t SPIN_RETRIES_BLOCK = SPIN_RETRIES_YIELD * 2;

constexpr size_t LFQ_CAPACITY = 65536;
constexpr size_t CACHE_LINE_SIZE = 64;
constexpr size_t LOG_FILE_SIZE = 100 * 1024 * 1024;
constexpr size_t MAX_CONNECTIONS = 10;
constexpr size_t PRICE_FLUCTUATION_RATE = 5;

} // namespace hft

#endif // HFT_COMMON_TYPES_HPP
