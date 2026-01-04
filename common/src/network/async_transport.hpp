/**
 * @author Vladimir Pavliv
 * @date 2026-01-03
 */

#ifndef HFT_COMMON_ASYNCTRANSPORT_HPP
#define HFT_COMMON_ASYNCTRANSPORT_HPP

#include "network/network_buffer.hpp"
#include "types.hpp"

namespace hft {

enum class IoResult : uint8_t { Ok, WouldBlock, Closed, Error };

template <typename T>
concept AsyncTransport = requires(T t, ByteSpan buf) {
  {
    t.asyncRx(buf, [](IoResult, size_t) {})
  };
  {
    t.asyncTx(buf, [](IoResult, size_t) {})
  };
  { t.close() } noexcept;
};
} // namespace hft

#endif // HFT_COMMON_ASYNCTRANSPORT_HPP