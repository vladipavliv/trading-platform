/**
 * @author Vladimir Pavliv
 * @date 2025-02-13
 */

#ifndef HFT_COMMON_FUNCTIONALTYPES_HPP
#define HFT_COMMON_FUNCTIONALTYPES_HPP

#include <expected>
#include <functional>
#include <optional>

#include "status_code.hpp"

namespace hft {
template <typename T>
using Expected = std::expected<T, StatusCode>;

template <typename T>
using Optional = std::optional<T>;

using Callback = std::function<void()>;

template <typename ArgType>
using CRefHandler = std::function<void(const ArgType &)>;

} // namespace hft

#endif // HFT_COMMON_FUNCTIONALTYPES_HPP
