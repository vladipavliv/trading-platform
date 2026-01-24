/**
 * @author Vladimir Pavliv
 * @date 2026-01-03
 */

#ifndef HFT_COMMON_TRANSPORT_HPP
#define HFT_COMMON_TRANSPORT_HPP

#include "container_types.hpp"
#include "functional_types.hpp"
#include "io_result.hpp"
#include "primitive_types.hpp"
#include "utils/handler.hpp"

namespace hft {

struct Functor {
  void operator()(IoResult) const {}
};

template <typename T>
concept Transportable = requires(T t, ByteSpan rxBuf, CByteSpan txBuf) {
  { t.syncRx(rxBuf) } -> std::same_as<IoResult>;
  { t.syncTx(txBuf) } -> std::same_as<IoResult>;
  { t.close() };
} && (
  requires(T t, ByteSpan rxBuf, CByteSpan txBuf) {
    { t.asyncRx(rxBuf, [](IoResult){}) } -> std::same_as<void>;
    { t.asyncTx(txBuf, [](IoResult){}) } -> std::same_as<void>;
  } ||
  requires(T t, ByteSpan rxBuf, CByteSpan txBuf) {
    { t.asyncRx(rxBuf, Functor{}) } -> std::same_as<void>;
    { t.asyncTx(txBuf, Functor{}) } -> std::same_as<void>;
  }
);
} // namespace hft

#endif // HFT_COMMON_ASYNCTRANSPORT_HPP
