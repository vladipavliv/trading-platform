/**
 * @file result.hpp
 * @brief Helper class to return errors
 *
 * @author Vladimir Pavliv
 * @date 2025-02-13
 */

#ifndef HFT_COMMON_RESULT_HPP
#define HFT_COMMON_RESULT_HPP

#include <variant>

#include "error_code.hpp"
#include "types.hpp"

namespace hft {

template <typename ValueType>
class Result {
public:
  Result() : mValue{ErrorCode::Empty} {}
  Result(ValueType value) : mValue(std::move(value)) {}
  Result(ErrorCode error) : mValue(error) {}

  bool ok() const { return std::holds_alternative<ValueType>(mValue); }
  const ValueType &value() const { return std::get<ValueType>(mValue); }
  ErrorCode error() const { return std::get<ErrorCode>(mValue); }
  ValueType extract() {
    if (ok()) {
      return std::get<ValueType>(std::move(mValue));
    }
    throw std::bad_variant_access{};
  }

private:
  std::variant<ValueType, ErrorCode> mValue;
};

} // namespace hft

#endif // HFT_COMMON_RESULT_HPP