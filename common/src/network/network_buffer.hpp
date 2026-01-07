/**
 * @author Vladimir Pavliv
 * @date 2026-01-04
 */

#ifndef HFT_COMMON_NETWORKBUFFER_HPP
#define HFT_COMMON_NETWORKBUFFER_HPP

#include "buffer_pool.hpp"
#include "container_types.hpp"
#include "primitive_types.hpp"

namespace hft {

class NetworkBuffer {
public:
  NetworkBuffer(Buffer &&b) : buffer_{std::move(b)} {}

  constexpr explicit operator bool() const noexcept { return buffer_.data != nullptr; }
  constexpr bool operator!() const noexcept { return buffer_.data == nullptr; }

  inline ByteSpan dataSpan() const { return ByteSpan{buffer_.data, buffer_.size}; }
  inline uint8_t *data() const { return buffer_.data; }
  inline size_t index() const { return buffer_.index; }
  inline size_t size() const { return buffer_.size; }
  inline void setSize(size_t size) { buffer_.size = size; }

private:
  Buffer buffer_;
};

} // namespace hft

#endif // HFT_COMMON_NETWORKBUFFER_HPP