/**
 * @author Vladimir Pavliv
 * @date 2026-01-07
 */

#ifndef HFT_COMMON_PTRTYPES_HPP
#define HFT_COMMON_PTRTYPES_HPP

#include <memory>

namespace hft {
template <typename T>
using SPtr = std::shared_ptr<T>;
template <typename T>
using UPtr = std::unique_ptr<T>;
template <typename T>
using WPtr = std::weak_ptr<T>;

// Const Reference Helper
template <typename T>
using CRef = const T &;

} // namespace hft

#endif // HFT_COMMON_PTRTYPES_HPP
