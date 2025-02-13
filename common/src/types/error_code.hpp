/**
 * @file
 * @brief
 *
 * @author Vladimir Pavliv
 * @date 2025-02-13
 */

#ifndef HFT_COMMON_ERROR_CODE_HPP
#define HFT_COMMON_ERROR_CODE_HPP

#include <stdint.h>

namespace hft {

enum class ErrorCode : uint8_t { Ok = 0U, Error = 1U };

} // namespace hft

#endif // HFT_COMMON_ERROR_CODE_HPP