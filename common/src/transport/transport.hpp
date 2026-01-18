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

namespace hft {

template <typename T, typename Handler = CRefHandler<IoResult>>
concept Transport = requires(T t, ByteSpan rxBuf, CByteSpan txBuf, Handler hndl) {
  { t.syncRx(rxBuf) } -> std::same_as<IoResult>;
  { t.syncTx(txBuf) } -> std::same_as<IoResult>;
  { t.asyncRx(rxBuf, std::move(hndl)) } -> std::same_as<void>;
  { t.asyncTx(txBuf, std::move(hndl)) } -> std::same_as<void>;
  { t.close() };
};
} // namespace hft

#endif // HFT_COMMON_ASYNCTRANSPORT_HPP
