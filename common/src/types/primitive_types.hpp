/**
 * @author Vladimir Pavliv
 * @date 2026-01-07
 */

#ifndef HFT_COMMON_PRIMITIVETYPES_HPP
#define HFT_COMMON_PRIMITIVETYPES_HPP

#include <atomic>
#include <chrono>
#include <cstdint>
#include <string>
#include <string_view>

namespace hft {

using Timestamp = uint64_t;
using ConnectionId = uint64_t;
using ObjectId = uint64_t;
using Token = uint64_t;
using CoreId = uint16_t;
using ThreadId = uint16_t;

using String = std::string;
using StringView = std::string_view;

using Port = uint16_t;
using MessageSize = uint16_t;

using Seconds = std::chrono::seconds;
using Milliseconds = std::chrono::milliseconds;
using Microseconds = std::chrono::microseconds;
using Nanoseconds = std::chrono::nanoseconds;

template <typename T>
using Atomic = std::atomic<T>;

using AtomicBool = std::atomic<bool>;
using AtomicUInt32 = std::atomic<uint32_t>;
using AtomicUint64 = std::atomic<uint64_t>;
using AtomicFlag = std::atomic_flag;

enum class State : uint8_t { On, Off, Error };

} // namespace hft

#endif // HFT_COMMON_PRIMITIVETYPES_HPP
