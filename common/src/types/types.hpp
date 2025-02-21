/**
 * @author Vladimir Pavliv
 * @date 2025-02-13
 */

#ifndef HFT_COMMON_TYPES_HPP
#define HFT_COMMON_TYPES_HPP

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
/**
 * @brief Prevent infinite loop while consuming elements from lock free Q
 * consume_all is not good enough as processing events in chunks is more efficient
 * @todo try increasing and test
 */
constexpr size_t LFQ_POP_LIMIT = 10;

#ifndef CACHE_LINE_SIZE
#define CACHE_LINE_SIZE 64
#endif

using String = std::string;
using StringRef = const std::string &;
using StringView = std::string_view;
using ObjectId = uintptr_t;
using ByteBuffer = std::vector<uint8_t>;
using SPtrByteBuffer = std::shared_ptr<ByteBuffer>;
using ThreadId = uint8_t;
using TimestampRaw = uint32_t;

} // namespace hft

#endif // HFT_COMMON_TYPES_HPP