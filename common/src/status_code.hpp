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
} // namespace hft

#endif // HFT_COMMON_STATUSCODE_HPP