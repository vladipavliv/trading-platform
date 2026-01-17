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

template <size_t N>
inline String fromArray(const char (&arr)[N]) {
  std::string_view view(arr, N);
  auto null_pos = view.find('\0');
  return String(view.substr(0, null_pos));
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

inline String formatNs(uint64_t ns) {
  if (ns < 1'000) {
    return std::format("{}ns", ns);
  } else if (ns < 1'000'000) {
    double us = ns / 1'000.0;
    return std::format("{:.2f}µs", us);
  } else {
    double ms = ns / 1'000'000.0;
    return std::format("{:.2f}ms", ms);
  }
}

/**
 * @brief Number thousandifier
 * @return Thousandified number. A number, that has gone through thousandification procedures
 */
template <std::integral T>
std::string thousandify(T value) {
  std::string s = std::to_string(value);
  int n = s.length() - 3;
  while (n > 0) {
    s.insert(n, ",");
    n -= 3;
  }
  return s;
}

} // namespace utils

using utils::toString;

} // namespace hft

#endif // HFT_COMMON_STRINGUTILS_HPP
