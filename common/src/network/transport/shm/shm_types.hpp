/**
 * @author Vladimir Pavliv
 * @date 2026-01-04
 */

#ifndef HFT_COMMON_SHMTYPES_HPP
#define HFT_COMMON_SHMTYPES_HPP

#include "primitive_types.hpp"

namespace hft {

enum class ShmTransportType : uint8_t { Upstream, Downstream, Datagram };

enum class ReactorType : uint8_t { Server, Client };

} // namespace hft

#endif // HFT_COMMON_SHMTYPES_HPP