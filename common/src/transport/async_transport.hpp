/**
 * @author Vladimir Pavliv
 * @date 2026-01-03
 */

#ifndef HFT_COMMON_ASYNCTRANSPORT_HPP
#define HFT_COMMON_ASYNCTRANSPORT_HPP

#include "container_types.hpp"
#include "io_result.hpp"
#include "primitive_types.hpp"

namespace hft {

template <typename T>
concept AsyncTransport = requires(T t, ByteSpan buf) {
  {
    t.asyncRx(buf, [](IoResult, size_t) {})
  };
  {
    t.asyncTx(buf, [](IoResult, size_t) {})
  };
  { t.close() };
};
} // namespace hft

#endif // HFT_COMMON_ASYNCTRANSPORT_HPP