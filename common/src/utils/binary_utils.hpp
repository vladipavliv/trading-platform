/**
 * @author Vladimir Pavliv
 * @date 2025-02-13
 */

#ifndef HFT_COMMON_BINARYUTILS_HPP
#define HFT_COMMON_BINARYUTILS_HPP

#include <boost/endian/arithmetic.hpp>

namespace hft::utils {
using LittleEndianUInt16 = boost::endian::little_uint16_at;
using LittleEndianUInt32 = boost::endian::little_uint32_at;
} // namespace hft::utils

#endif // HFT_COMMON_BINARYUTILS_HPP
