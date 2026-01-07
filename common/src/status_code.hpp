/**
 * @author Vladimir Pavliv
 * @date 2025-04-20
 */

#ifndef HFT_COMMON_STATUSCODE_HPP
#define HFT_COMMON_STATUSCODE_HPP

#include <cstdint>

namespace hft {
/**
 * @todo Extend error codes
 */
enum class StatusCode : uint8_t { Ok, Error, DbError, AuthUserNotFound, AuthInvalidPassword };

inline String toString(const StatusCode &code) {
  switch (code) {
  case StatusCode::Ok:
    return "Ok";
  case StatusCode::Error:
    return "Error";
  case StatusCode::DbError:
    return "DbError";
  case StatusCode::AuthUserNotFound:
    return "AuthUserNotFound";
  case StatusCode::AuthInvalidPassword:
    return "AuthInvalidPassword";
  default:
    return std::format("unknown");
  }
}
} // namespace hft

#endif // HFT_COMMON_STATUSCODE_HPP