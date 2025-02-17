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
#include <stdint.h>
#include <string>
#include <vector>

namespace hft {

using String = std::string;
using StringRef = const std::string &;
using StringView = std::string_view;
using ObjectId = uintptr_t;

constexpr size_t BUFFER_SIZE = 1024 * 8;
constexpr size_t EVENT_QUEUE_SIZE = 1024;

using ByteBuffer = std::vector<uint8_t>;
using SPtrByteBuffer = std::shared_ptr<ByteBuffer>;

template <typename Arg>
using CRefHandler = std::function<void(const Arg &)>;

using ThreadId = uint8_t;

#ifndef CACHE_LINE_SIZE
#define CACHE_LINE_SIZE 64
#endif

template <typename... ValueTypes>
struct Padding {
  static constexpr size_t DataSize = (sizeof(ValueTypes) + ... + 0);
  static constexpr size_t PaddingSize =
      (CACHE_LINE_SIZE > DataSize) ? (CACHE_LINE_SIZE - DataSize) : 0;

  std::array<char, PaddingSize> padding{};
};

} // namespace hft

#endif // HFT_COMMON_TYPES_HPP