/**
 * @author Vladimir Pavliv
 * @date 2026-01-07
 */

#ifndef HFT_COMMON_PARSEUTILS_HPP
#define HFT_COMMON_PARSEUTILS_HPP

#include <charconv>
#include <chrono>
#include <cstdlib>
#include <string>
#include <string_view>
#include <vector>

namespace hft::utils {

template <typename T>
inline auto split(std::string_view input) -> std::vector<T> {
  std::vector<T> result;

  size_t start = 0;
  while (start < input.size()) {
    size_t end = input.find(',', start);
    if (end == std::string_view::npos) {
      end = input.size();
    }
    std::string_view token = input.substr(start, end - start);
    const size_t first = token.find_first_not_of(" \t");
    if (first != std::string_view::npos) {
      const size_t last = token.find_last_not_of(" \t");
      token = token.substr(first, (last - first + 1));

      T value{};
      auto [ptr, ec] = std::from_chars(token.data(), token.data() + token.size(), value);

      if (ec == std::errc{}) {
        result.push_back(value);
      }
    }
    start = end + 1;
  }
  return result;
}

inline std::string getEnvVar(const std::string &varName) {
  const char *value = std::getenv(varName.c_str());
  return value == nullptr ? "" : value;
}

} // namespace hft::utils

#endif // HFT_COMMON_PARSEUTILS_HPP
