/**
 * @author Vladimir Pavliv
 * @date 2025-02-13
 */

#ifndef HFT_COMMON_RESULT_HPP
#define HFT_COMMON_RESULT_HPP

#include <variant>

#include "status_code.hpp"
#include "types.hpp"

namespace hft {

/**
 * @todo Remove this, use expected
 */
template <typename ValueType>
struct Result {
  Result() : code{StatusCode::Empty} {}
  Result(ValueType value) : value{std::move(value)}, code{StatusCode::Ok} {}
  Result(StatusCode error) : code{error} {}

  bool ok() const { return code == StatusCode::Ok; }

  ValueType value;
  StatusCode code;
};

} // namespace hft

#endif // HFT_COMMON_RESULT_HPP