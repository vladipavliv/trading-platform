/**
 * @author Vladimir Pavliv
 * @date 2025-04-20
 */

#ifndef HFT_COMMON_STATUSCODE_HPP
#define HFT_COMMON_STATUSCODE_HPP

#include <cstdint>

namespace hft {

enum class StatusCode : uint8_t {
  Ok = 0U,
  Error = 1U,
  DbError = 2U,
  AuthUserNotFound = 3U,
  AuthInvalidPassword = 4U
};
}

#endif // HFT_COMMON_STATUSCODE_HPP