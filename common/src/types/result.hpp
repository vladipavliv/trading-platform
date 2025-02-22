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

template <typename ValueType>
class Result {
public:
  Result() : mValue{ValueType{}, StatusCode::Empty} {}
  Result(ValueType value) : mValue{std::move(value), StatusCode::Ok} {}
  Result(StatusCode error) : mValue{ValueType{}, error} {}

  bool ok() const { return mValue.second == StatusCode::Ok; }
  operator bool() const { return ok(); }
  const ValueType &value() const { return mValue.first; }
  StatusCode error() const { return mValue.second; }

private:
  std::pair<ValueType, StatusCode> mValue;
};

} // namespace hft

#endif // HFT_COMMON_RESULT_HPP