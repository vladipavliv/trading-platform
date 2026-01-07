/**
 * @author Vladimir Pavliv
 * @date 2026-01-07
 */

#ifndef HFT_COMMON_STRINGUTILS_HPP
#define HFT_COMMON_STRINGUTILS_HPP

#include <algorithm>
#include <concepts>
#include <cstring>
#include <string>
#include <string_view>
#include <vector>

#include "primitive_types.hpp"
#include "ticker.hpp"

namespace hft {
namespace utils {

template <typename T>
concept StdToStringable = requires(T a) { std::to_string(a); };

inline String toString(const utils::StdToStringable auto &val) { return std::to_string(val); }

template <typename T>
String toString(const std::vector<T> &vec) {
  String out = "[";
  for (size_t i = 0; i < vec.size(); ++i) {
    out += toString(vec[i]) + (i == vec.size() - 1 ? "" : ",");
  }
  out += "]";
  return out;
}

inline String toLower(String str) {
  std::transform(str.begin(), str.end(), str.begin(),
                 [](unsigned char c) { return std::tolower(c); });
  return str;
}

inline Ticker toTicker(StringView str) {
  Ticker ticker{};
  std::memcpy(ticker.data(), str.data(), std::min(str.size(), TICKER_SIZE));
  return ticker;
}

inline bool isEmpty(const Ticker &ticker) {
  for (auto c : ticker)
    if (c != '\0')
      return false;
  return true;
}
} // namespace utils

using utils::toString;

} // namespace hft

#endif // HFT_COMMON_STRINGUTILS_HPP
