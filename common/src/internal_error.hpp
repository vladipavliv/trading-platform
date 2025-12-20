/**
 * @author Vladimir Pavliv
 * @date 2025-12-20
 */

#ifndef HFT_COMMON_INTERNALERROR_HPP
#define HFT_COMMON_INTERNALERROR_HPP

#include "status_code.hpp"
#include "types.hpp"

namespace hft {

struct InternalError {
  StatusCode code;
  String what;
};

} // namespace hft

#endif // HFT_COMMON_INTERNALERROR_HPP