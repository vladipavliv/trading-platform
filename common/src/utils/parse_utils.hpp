/**
 * @author Vladimir Pavliv
 * @date 2026-01-07
 */

#ifndef HFT_COMMON_PARSEUTILS_HPP
#define HFT_COMMON_PARSEUTILS_HPP

#include <charconv>
#include <cstdlib>
#include <string>
#include <type_traits>
#include <vector>

namespace hft::utils {

template <typename T>
inline auto split(const std::string &input) -> std::vector<T> {
  std::vector<T> res;
  std::stringstream ss(input);
  std::string token;

  while (std::getline(ss, token, ',')) {
    if constexpr (std::is_same_v<T, std::string>) {
      res.push_back(std::move(token));
    } else {
      res.push_back(std::stoi(token));
    }
  }
  return res;
}

inline std::string getEnvVar(const std::string &varName) {
  const char *value = std::getenv(varName.c_str());
  return value == nullptr ? "" : value;
}

} // namespace hft::utils

#endif // HFT_COMMON_PARSEUTILS_HPP
