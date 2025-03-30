/**
 * @author Vladimir Pavliv
 * @date 2025-02-13
 */

#ifndef HFT_COMMON_TYPES_HPP
#define HFT_COMMON_TYPES_HPP

#include <cstdint>
#include <memory>
#include <stdint.h>
#include <string>
#include <vector>

namespace hft {

using String = std::string;
using CRefString = const std::string &;
using StringView = std::string_view;
using ObjectId = size_t;
using ByteBuffer = std::vector<uint8_t>;
using SPtrByteBuffer = std::shared_ptr<ByteBuffer>;
using ThreadId = uint8_t;
using CoreId = uint8_t;
using Thread = std::jthread;
using TimestampRaw = uint32_t;
using LogLevel = spdlog::level::level_enum;

} // namespace hft

#endif // HFT_COMMON_TYPES_HPP